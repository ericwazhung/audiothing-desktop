/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//errno_handleError.h 0.15-2
// Checks errno, if non-zero prints the error and its string
//               if exit = 0, clears the error
//                  exit = 1, is the equivalent of running "return 1"
//                            in the calling function
//
// Basically, run this after any i/o to check errno
//            For non-blocking i/o there'll be a "resource unavailable"
//             error, which isn't a problem and should just be cleared
//             *without* entering this function...

//0.15-2 adding errno_findError()
//0.15-1 note re: can't be used in void functions...
//0.15 Error Return-Val now selectable (removes need for wrapper-functions)
//0.10-3 Modifying for wrapper-functions to send "string" as a string of
//       chars
//0.10-2 Modifying to add errtmp, just in case something modifies errno
//0.10-1 TODO: Rather than use printf, try perror()
//             see man 2 intro
//0.10 Stolen from cTools/stdinNonBlock.c

#ifndef _ERRNO_HANDLEERROR_H_
#define _ERRNO_HANDLEERROR_H_

#include <stdio.h>
#include <errno.h>
#include <string.h> //Needed for strerror()

//If returnOnError == 0, then doesn't cause the containing-function to
//return
#define ERRHANDLE_DONT_RETURN 0


//NOTE: Without using the optimizer, this CANNOT BE USED in a "void"
//function...
// even with returnOnError=ERRHANDLE_DONT_RETURN
//Lame.
#define errno_handleError(string, returnOnError) \
      if(errno)   \
{\
   typeof(errno) errtmp = errno; \
         printf( "%s Error: errno=%d: '%s'\n", string, errtmp, \
               strerror(errtmp)); \
         if(returnOnError) \
            return returnOnError; \
         else \
            errno = 0; \
}


//errno_findError is identical to errno_handleError, except that it's
//replaced with *nada* if ERRNO_FIND_ERROR_ENABLED is not defined, or false


#if(defined(ERRNO_FIND_ERROR_ENABLED) && ERRNO_FIND_ERROR_ENABLED)
 #define errno_findError(string, ret) \
            errno_handleError((string), (ret))
#else
 #define errno_findError(string, ret)
#endif

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
 * /home/meh/audioThing/7p16-git/_commonCode_localized/__std_wrappers/errno_handleError/0.15/errno_handleError.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
