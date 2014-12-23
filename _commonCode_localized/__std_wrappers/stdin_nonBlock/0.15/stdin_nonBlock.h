/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//stdin_nonBlock.h 0.15

//0.15 continuation of 0.10-7's issue... which'll be relevent elsewhere:
//     in audioThing, using PortAudio:
//     Pa_WriteStream returns with errno=11 
//        "resource temporarily unavailable"
//     THIS PREVENTS stdinNB_get() from functioning.
//     clearing the error allows stdinNB to function properly
//     THIS SORT OF THING SHOULD probably be handled external to stdinNB
//     BUT: It makes for some confusing issues, like these...
//          where, portAudio is actually functioning, and so should be
//          stdinNB, but stdinNB is cripled...
//     INSTEAD: check for these errors prior to reading from stdin
//     
//0.10-7 a/o audioThing-7p13:
//       audioThing reports:
//         Couldn't open stdin. Error: errno=88: 'Socket operation on
//         non-socket'
//       Is it possible the hard-coded FID=0, here, is causing issues?
//       Added some overkill, but the notes won't hurt.
//       TURNS OUT: duh. errno was set *before* entering stdinNB_init()
//       ALSO: adding note from 'man stdin' re: potentially not
//       line-buffering (TODO!)
//
//0.10-6 Since hsStowawayKB was uploaded, this is apparently needed for one
//       of the test-apps...
//       adding mehPL, some usage-notes, and uploading.
//0.10-5 switching to errno_handleError0.15
//0.10-4 A/O Debian (for PUAT 0.75's 'test' not working (for Tiny84:
//							'resolutionDetector'))
//       FIXING STDIN_NONBLOCK NO LONGER WORKS (with Debian)
//       Looking Into It.
//       errno=11: 'Resource temporarily unavailable' (was 35 on OSX)
//       TODO: look into 'man unlocked_stdio' per 'man stdio'
//       see 'man errno.h' which reports error definitions...
//       Should be fixed, now, using 'EAGAIN' instead of magic-number
//0.10-3 TODO?
//       a/o audioThing7p9-1-27-14:
//       Check it out for string-handling
//       And also for cmd-input with real-time update on stdout
//       Also: look into 'man termcap'
//             (discovered via tcsh->echotc)
//0.10-2 TODO? May be related to below...
//             a/o spinner0.10: uses clearerr(stdin) instead of test/main.c
//             which does it differently...
//0.10-1 TODO: Sometimes (?!) Error 60: Operation Timed Out
//             Rather than 35, or due to something else?
//0.10 First version, Stolen from cTools/stdinNonBlock.c
//     Might need to do some editing based on errorHandling, etc.
//     problems found in tcnter/test, polled_ua[r/t]/test, etc.


// Basic Functionality:
//  NOTE: this still requires return be pressed before the data's available
//        Return *will* be emitted by stdinNB_getChar()
//        So in most cases we'll get at least two chars (in two calls)
// main()
// {
//   stdinNB_init();
//
//   while(1) {
//     int kbChar = stdinNB_getChar();
//     switch(kbChar) {
//       case -1:
//          //do nothing
//          break;
//       case 'a': ...
//     }
//    
//     ..Do other main-loop things, without waiting for keyboard input..
//   }
// }
//
// It's in NO WAY fully-functional... It still requires "Return" to
// be pressed before the system will recognize a keypress
// In this way, if an entire line of text is entered before pressing
// "Return," stdinNB_getChar() will return the characters one-by-one
// in several calls.
//
// Its main purpose is to allow for the main() loop to run without waiting
// for keyboard input.
// And is used somewhat extensively in most of my PC-based projects.


//TODO Look into:
// Re: Line-Buffering (requirement of "Return")
// From: 'man stdin'
// The buffer‐ ing mode of the standard streams (or any other stream) can
// be changed using  the  setbuf(3)  or  setvbuf(3) call.  Note that in 
// case stdin is associated with a terminal, there may also be input 
// buffering  in the terminal  driver, entirely unrelated to stdio 
// buffering.  (Indeed, normally terminal input is line buffered  in  the
// kernel.)   This kernel input  handling can be modified using calls like
// tcsetattr(3); see also stty(1), and termios(3).

//TODO ALSO:
// From: 'man getchar'
// It  is  not  advisable  to  mix calls to input functions from the stdio
// library with low-level calls to read(2) for the file descriptor associ‐
// ated  with  the  input  stream;  the results will be undefined and very
// probably not what you want.

// getchar should be usable here, but maybe has something to do with
// audioThing's flakiness...?
// So, looking into getc and fgetc:
// http://stackoverflow.com/questions/18480982/getc-vs-fgetc-what-are-the-major-differences
// and 'man getchar':
// getc()  is equivalent to fgetc() except that it may be implemented as a
// macro which evaluates stream more than once.
//
// means that e.g. getc(f++) might result in f++ multiple times.
//
// stdin is slow, no need to use a macro, here.

#ifndef _STDIN_NONBLOCK_H_
#define _STDIN_NONBLOCK_H_

#include <stdio.h>
//#include <errno.h>
#include <fcntl.h> //file control for open/read
#include "../../errno_handleError/0.15/errno_handleError.h"

int stdinNB_init(void)
{
   //NOTES:
   // fcntl(int fd, ...)
   // extern FILE *stdin
   // STDIN_FILENO (in unistd.h)
   // "Note  that  mixing  use  of  FILEs and raw file descriptors can
   // produce unexpected results and should generally be avoided." -- 'man
   // stdin'

   // fileno(FILE* ...) creates an int fd from a FILE*
   // This is probably overkill here, but lessee


   errno_handleError("errno was non-zero PRIOR TO entry of stdinNB_init()"
                     "\n IGNORING that errno! Debug that shizzle!", 
                        ERRHANDLE_DONT_RETURN);

	// Set STDIN non-blocking (Still requires Return)
	//int flags = fcntl(0, F_GETFL);
	int flags = fcntl(fileno(stdin), F_GETFL);
	flags |= O_NONBLOCK;
	//fcntl(0, F_SETFL, flags);
	fcntl(fileno(stdin), F_SETFL, flags);

	errno_handleError("Couldn't open stdin.", 1);

	return 0;
}

//Either returns a character stored in an int
// or returns -1 if no data's available
int stdinNB_getChar(void)
{
	unsigned char kbChar;


   //a/o 0.15:
   // Ignoring errno's that occurred from other sources
   // The only errno that should interfere with reception of a character
   // from stdin is the same tested-here...
   // So, realistically, maybe, this should only apply to that one...?
   if(errno)
   {
      static wasDisplayed = FALSE;

      if(!wasDisplayed)
      {
         errno_handleError(
               "errno was non-zero PRIOR TO entry of stdinNB_getChar() \n"
               " IGNORING AND CLEARING that errno!\n"
" FURTHER: stdinNB_getChar() will CONTINUE TO clear such prior errors\n"
"  WITHOUT FURTHER WARNING\n"
               " Debug that shizzle!\n", 
                        ERRHANDLE_DONT_RETURN);
      
         wasDisplayed=TRUE;
      }
      else
         errno = 0;
   }


	kbChar = fgetc(stdin); //getchar();

	//This error (35) is for MacOS (10.5.8)
	//"Resource Temporarily Unavailable" isn't an error...
	//Debian replies: errno=11: 'Resource temporarily unavailable'
//This is defined in 'man errno.h' as:
//  EAGAIN          Resource temporarily unavailable
// (and was verified as '11' in 'gcc -E -dM ...'
#ifdef EAGAIN
	if(errno == EAGAIN)
#else
	#error "Not gonna let you get away with just a warning..."
	#error " 'EAGAIN' is not defined, so we're defaulting to #35"
	#error "  which is the value on MacOS 10.5.8"
	#error "These errors can be turned into warnings, if you're cautious"
   #error "ALSO, see above!"
	if(errno == 35)
#endif
	{
		errno = 0;
		return -1;
	}
	else
		return (int)kbChar;
}

#endif

/* mehPL:
 *    I would love to believe in a world where licensing shouldn't be
 *    necessary; where people would respect others' work and wishes, 
 *    and give credit where it's due. 
 *    A world where those who find people's work useful would at least 
 *    send positive vibes--if not an email.
 *    A world where we wouldn't have to think about the potential
 *    legal-loopholes that others may take advantage of.
 *
 *    Until that world exists:
 *
 *    This software and associated hardware design is free to use,
 *    modify, and even redistribute, etc. with only a few exceptions
 *    I've thought-up as-yet (this list may be appended-to, hopefully it
 *    doesn't have to be):
 * 
 *    1) Please do not change/remove this licensing info.
 *    2) Please do not change/remove others' credit/licensing/copyright 
 *         info, where noted. 
 *    3) If you find yourself profiting from my work, please send me a
 *         beer, a trinket, or cash is always handy as well.
 *         (Please be considerate. E.G. if you've reposted my work on a
 *          revenue-making (ad-based) website, please think of the
 *          years and years of hard work that went into this!)
 *    4) If you *intend* to profit from my work, you must get my
 *         permission, first. 
 *    5) No permission is given for my work to be used in Military, NSA,
 *         or other creepy-ass purposes. No exceptions. And if there's 
 *         any question in your mind as to whether your project qualifies
 *         under this category, you must get my explicit permission.
 *
 *    The open-sourced project this originated from is ~98% the work of
 *    the original author, except where otherwise noted.
 *    That includes the "commonCode" and makefiles.
 *    Thanks, of course, should be given to those who worked on the tools
 *    I've used: avr-dude, avr-gcc, gnu-make, vim, usb-tiny, and 
 *    I'm certain many others. 
 *    And, as well, to the countless coders who've taken time to post
 *    solutions to issues I couldn't solve, all over the internets.
 *
 *
 *    I'd love to hear of how this is being used, suggestions for
 *    improvements, etc!
 *         
 *    The creator of the original code and original hardware can be
 *    contacted at:
 *
 *        EricWazHung At Gmail Dotcom
 *
 *    This code's origin (and latest versions) can be found at:
 *
 *        https://code.google.com/u/ericwazhung/
 *
 *    The site associated with the original open-sourced project is at:
 *
 *        https://sites.google.com/site/geekattempts/
 *
 *    If any of that ever changes, I will be sure to note it here, 
 *    and add a link at the pages above.
 *
 * This license added to the original file located at:
 * /home/meh/_commonCode/__std_wrappers/stdin_nonBlock/0.10/stdin_nonBlock.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
