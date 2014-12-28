/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */







//LICENSE ADDENDUM:
//'audioThing-desktop' uses PortAudio for its audio-playback functionality.
//'audioThing-desktop's audio-playback code is derived from a PortAudio
// example. Please see (and do not delete) the file portAudioLicense.txt




//#define ERRNO_FIND_ERROR_ENABLED TRUE
#define BYTES_PER_SAMPLE   2

#warning "TODO: Another button for text-only memos..."
#warning "TODO: Capslock on via Shift-Caps, off via Caps"
//#warning "TODONE: Show Header Information for auTx files"
//#warning "TODONE: AutoDetect file type (auT, auTx NOT via extension"
#warning "TODO: Add eraseUT as an option... maybe also customUT?"
#warning "TODO: button for 'try to keep from here' ish... e.g. going to bed"

//System Requirements:
// libportaudio
//   The naming-scheme is a bit confusing.
//    'audioThing-desktop' requires PortAudio 19 (aka API v2)
//    In Debian, it works with the following packages:
//             libportaudio2 (19+svn20111121-1)
//             portaudio19-dev (19+svn20111121-1)
//             (portaudio19-doc)
//  In MacOSX: It's been a while and I don't recall how I installed 
//   PortAudio, was it via macports?

//This is known to work on:
//   Debian Wheezy (x86) 
//And known to have worked on:
//   MacOSX 10.5.8 (PPC) 
//     --It was last used on this system only a few versions ago
//     --The vast-majority of this code was developed on this system



#ifndef __linux
#error "On OSX: This error can be removed very easily... see the comments below it, in the source code. On Windows, things may be difficult, as this requires direct-access to the SD-Card (without a mountable file-system)"
 //My OSX system has died, so I cannot test its #defines...
 // So, define this as appropriate (TRUE if you're using OSX)
 // (And comment-out the error, above)
 #define __MYDEF_MAC_OS_X__ FALSE


 //WINDOWS: Because this requires direct-access to the SD-Card
 //         (Because the SD-Card is formatted without a filesystem)
 //         I have no idea what you'd have to do... Sorry!
 //         FAT32 on the 'audioThing' device *may* be in the works...
 //         Stay tuned....?
#endif




// Per "man fseeko" on Debian:
// "On many architectures both off_t and long are 32-bit types, but  compi‐
//  lation with
//       #define _FILE_OFFSET_BITS 64
//  will turn off_t into a 64-bit type."
//
// It must be 32-bit by default, as blockCount*512 was returning a 
//  negative-value
// And, surprisingly, now it doesn't necessitate ioctl() calls to determine
//  the block-size...?
// Unrelated note: off_t is apparently *signed*
//                 as ftello returns -1 if there's an error
#if(defined(__linux) && __linux)
 #define _FILE_OFFSET_BITS 64
#endif



//v7p13:
//  This is the first version since switching over to Debian
//  (a/o audioThingAVR v51)
//  Several of the tools used here, especially those for interacting with
//   the device directly, were OSX (or BSD?) specific.
//  Making notes, and looking into necessary modifications
//  It appears the endianness in x86 Debian does not match that of
//  audioThing[AVR], so adding freadSamples()
//  Pa_WriteStream() on debian causes an errno that interferes with stdinNB
//  This is sorta fixed.


//a/o v7p12: This note added long-after the following warning...
// The idea, as I recall, being that after loopNum counts away from 0xf it
// appears as the next bootnum... so each marked chunk (after the first
// alleged-"boot") might in fact be from a bootNum prior than the one in
// the usage-table... 
// if, e.g., the loop count was 20 on the first boot, 
// we'd have bootNum=1, loopNum=4
// Since it's a "new boot" there's no way to know when the event occurred
// WRT the first boot. But, that info *is* possible to determine, if we
// somehow know that there wasn't an actual reboot...
// Could follow the warning, below, and just include the usageTable in each
// extracted file, so it could be determined by hand...
// OR Possibly:
// Include 16 nibbles (8 bytes) for a BootTable...?
// (Needn't be in EEPROM, right? Maybe only on SD?)
// FURTHER:
//  ADD a WDT...
//   If BadDataResponse, reboot, but don't increment bootNum...?
//   Also, attempt to reuse remainder of chunk? (so-as not to lose track of
//   time)
#warning "TODO: BIG ONE: Extract usageTable info, so apparent later-boots can be recovered, when really they're just loops"
//#warning "TODO: Extract consecutive (but fragmented) sections as one"





#include <fcntl.h>
#include <sys/ioctl.h>

#if(defined(__MYDEF_MAC_OS_X__) && __MYDEF_MAC_OS_X__)
 #include <sys/disk.h>   //Apple... Block-Getting ioctls
#elif(defined(__linux) && __linux)
 #include <linux/fs.h>  //BLKGETSIZE
#endif


#include <limits.h> //INT_MAX, etc.
#include <ctype.h> //isprint()

#include <stdlib.h>  //exit()

#include <sys/errno.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <math.h>

#if(defined(__MYDEF_MAC_OS_X__) && __MYDEF_MAC_OS_X__)
 //This is how it was used on MacOS... Probably doesn't matter, but we'll
 //leave it...
 //Doesn't seem to matter on Debian.
 #include "portaudio.h"
#else
 #include <portaudio.h>
#endif

#include <sys/types.h> //for stat
#include <sys/stat.h>  //ditto
#include <libgen.h>     //basename()
#include <time.h>       //ctime
#include <sys/time.h>   //gettimeofday


#include _STDIN_NONBLOCK_HEADER_


//Stolen from hsStowawayKB.h
//a/o 7p11 this hasn't been defined here prior
//         and may not be fully-implemented, yet, esp: with nokia keypad.
// audioThing(AVR)/main.c lists nokia keypad memo-key as 'I'
#define HSSKB_MEMO_RETURN  0xAB  //((uint8_t)'«')
#define NKP_MEMO_RETURN 'I'

#define NUM_SECONDS         (5)
#define SAMPLE_RATE         (44100) //(19230) -> Output Underflow
#define FRAMES_PER_BUFFER   (1024)

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE  (FRAMES_PER_BUFFER)// (200)


#define FILE_SAMPLE_RATE   19230
#define FILE_SAMPLE_BYTES  2


#define VERBOSITY_MAX   INT_MAX
int verbosity = VERBOSITY_MAX;

//verbosity must be greater than or equal to "level" to print...
#define v_printf(level, ...) \
   ( ((level) <= verbosity) ? printf(__VA_ARGS__) : 0 )

#define v_check(level) \
   if((level) <= verbosity)

FILE *fid;

#define HEADER_SIZE 512
uint8_t headerData[HEADER_SIZE];
#define USAGETABLE_SIZE  256
uint8_t *ut = &(headerData[256]); //[USAGETABLE_SIZE];

//Realistically, this could be removed in favor of the fragList...
int sortedUTindices[USAGETABLE_SIZE+1] = { [0 ... USAGETABLE_SIZE] = -1 };
int sortUT_index = 0;


#define HEADER_SIZE  512

#define NO_HEADER    0
#define auT_HEADER   1
#define UT_HEADER    auT_HEADER
#define auTx_HEADER  2
int loadHeader(void);

int loadUT(void); //int *loopMax);
int sortUT(void); //int loopMax);
void printFragList(void);
int getFragBytes(int fragListIndex, off_t *startByte, off_t *endByte);

size_t freadSamples(uint16_t *sampleBuffer, size_t numSamples, FILE* fid);

//This will return the number of chunks *not including* the one we're
// looking for... e.g. utPos = 0, ut[0] = 1, will return 0
// Includes wraparound loopCounting, whatever.
uint32_t chunksSinceBoot_fromUTPosition(uint8_t utPos);

typedef struct ___BLAH2___
{
   int16_t firstChunk;  //ut Position (or -1 if this is the last fragment)
   int16_t lastChunk;
   int16_t loopNum;     //loopNum as found in ut[utPosition]
   uint8_t linkedWithNext;    //TRUE if this fragment links to the next
                        // (e.g. part of a memo that crosses several fragments)
   uint8_t linkedWithLast;

} fragment2_t;

fragment2_t fragList2[USAGETABLE_SIZE] = {
                           [ 0 ... (USAGETABLE_SIZE-1) ] = {-1, -1, -1, FALSE} };



//Each memo could occur across several fragments
// (and even wrap back to the beginning of the card in the next loopNum)
// so memoFragments[memoNum][fragmentNum] = fragListIndex
// if the value is -1, it's the end of the list...
int memoFragments[USAGETABLE_SIZE][USAGETABLE_SIZE] =
   { [ 0 ... (USAGETABLE_SIZE-1) ][ 0 ... (USAGETABLE_SIZE-1) ] = -1 };



//This is the sample-rate used on the original audioThing
// And, most-likely, will only be necessary when reading cards/files
// written by versions earlier than v60(61?)
#define SAMPLE_RATE_DEFAULT 19230

int sampleRate = SAMPLE_RATE_DEFAULT;
int srAssumed = 1;

off_t numBlocksPerChunk;


off_t calcBytesFromChunks(uint32_t chunks)
{
   return (off_t)(chunks) * numBlocksPerChunk * 512;
}

//This should also work with chunkNums outside the usage-table...
// e.g. chunk-differences...
// e.g. those from chunksSinceBoot
// ERM: Are the above e.g.'s for calcBytesFromChunks, not this?
off_t startingByteNumFromChunk(uint32_t chunkNum)
{
   //Don't overwrite the usage-table!
   if(chunkNum == 0)
      return 512;
   else
      return calcBytesFromChunks(chunkNum);
         
   //(off_t)(chunkNum) * numBlocksPerChunk * 512;
}

//This is the file-byte, not the recorded-byte number
// IOW this can't be used directly for calculating the amount of time since
// it last booted
off_t lastByteInChunk(uint32_t chunkNum)
{
   return calcBytesFromChunks(chunkNum+1)-1;
}

void printFragInfo(int fragNum, int printHeader, int printBootNum,
                               int printLoopNum, int printFileBytes);

int printPosition(off_t position_o, uint8_t withReturn);

void printPosition2(uint8_t useUT, uint8_t startingChunkNum,
                     off_t seek_o, uint8_t printReturn);

int timeStringsFromFrag(int fragNum, char *startTimeString, 
                        char *endTimeString, int linkify);

int timeStringFromBytes(char * string, off_t bytes);
int printTimeFromBytes(off_t bytes);

#define SCAN_UNHANDLED  (UINT64_MAX)
uint64_t scanPosition(char *string);


void printProgressBar(off_t done, off_t total);

void printTimeEstimate(off_t done, off_t total, struct timeval startTime)
{
   
   struct timeval nowTime;
   gettimeofday(&nowTime, NULL);

   nowTime.tv_sec -= startTime.tv_sec;
   nowTime.tv_usec -= startTime.tv_usec;

   uint64_t usecTotal = (uint64_t)nowTime.tv_sec*1000000
                                       + nowTime.tv_usec;

   //The transfer rate in bytes/usec is:
   double transferRate = (double)done/(double)usecTotal;

   double remainingTime = (double)(total - done)
                                       / transferRate;

   int remainingMinutes = remainingTime/1000000/60;
   int remainingSeconds = remainingTime/1000000
                                        -remainingMinutes*60;

   printf(" %d:%02d", remainingMinutes, remainingSeconds);
}

void displayPlaybackUsage(void);

void displayUsage(void)
{
   printf("audiothing [args] <file>\n"
          "   -h . . . . display this help message (and exit)\n"
          "   -V=<float> set the playback volume: (1.0=no change)\n"
          "   -v=<int> . verbosity: (Default = max verbosity)\n"
          "              E.G. '-v=1000' -> Really Verbose\n"
          "   -s . . . . search for text (no playback)\n"
          "   -sr  . . . search *readable*\n"
          "   -u . . . . use usage-table (default)\n"
          "   -nu  . . . don't use usage-table\n"
          "   -e=dir . . extract to dir\n"
          "\n"
          "   -l=<...> . limit search to (byte position, time...)\n"
          "   -b=<...> . begin searching from (byte position, time...)\n"
          "       -l= and -b= need to be better-described...\n"
          "                   som'n like: '00h01m27s' should work\n"
          "                   or: '0x000138' byte-position\n"
          "\n"
          "Playback:\n"
          " Except for -e and -s, audio playback will occur\n");

   displayPlaybackUsage();
}

void displayPlaybackUsage(void)
{
   printf(
          "  During playback the following commands can be entered:\n"
          "    h<return>          Display this list of playback-options\n"
          "    v=<float><return>  Set the volume to <float> (1=normal)\n"
          "    s=<float><return>  Set the playback speed to <float>\n"
          "                       (1=normal, 0.1=min, 10=max)\n"
          "    q<return>          Quit playback\n");
}

float volume=1.;

//v7p22: Thoughts on silence when the volume's too high--likely due to an
//uncentered signal that, when amplified a lot, ends up being a
//reasonably-loud signal but with an offset that pushes it past the rail...
//
//e.g. currently, the range is 464-470, obviously not centered around 512
//     at v=10 this is audible, surprisingly, and relatively clear
//     but v=12 results in near-silence.
// So, instead, have a variable offset, similar to AC-coupling.
//     The sampleOffset, then, is the (roughly) DC-offset of the incoming
//     samples. (464+470)/2 = 468.
// Except:
//     The DC-offset seems to vary as the AC part increases.
//     So, I guess we need some way of varying it slowly
//
//     The basic idea is a simple filter... something like this:
//    _   _
//   / \_/ \  >---/\/\/\---+----> DC-Offset
//                         |
//                       -----
//                       -----
//                         |
//                        GND
//
//   In software this can be, essentially:
//   (for each sample)
//   Out = old + (in-old)/som'n
//
//   where som'n basically affects the amount of effect the new sample has
//   on the output... which in turn affects how much time, e.g., it takes
//   for the output to match a constant input. (I suppose: T=RC?)
//
//        v---offsetDelay----v
//         __________________|_____________ 
//        |       __----¯¯¯¯¯|
//        |   .-¯¯           |--> Increasing "som'n" shifts this right
//        | .¯
//        |/
// -----------------------------------------> t 
//Since audioThing is 10-bits, the largest value being 1023, then the
//initial DC-offset assumption is centered at 512...
//This'll be adjusted as necessary.
float sampleOffset = 512;

//So this is "som'n"
// And, just completely winging it, I'm guessing it's something like 3
// times the number of constant DC samples until the output matches the
// input...
//
// So, e.g. say you want the DC-offset to take a full second (might be too
// much, we'll see) then it should be, guessing, about 3*the sample-rate.
#define OFFSET_DELAY_MULTIPLIER 3
// offsetDelay will be adjusted if sampleRate is...
float offsetDelay = (OFFSET_DELAY_MULTIPLIER * SAMPLE_RATE_DEFAULT);

off_t fileSize;


void searchForText(int useUsageTable, off_t searchBegin_o, off_t searchLimit_o);


//This should only be called sequentially... and/or at the beginning of a
//sample-pair...?
// ignoreAlignmentErrors is for, e.g., searching, which may arrive at any
// point in a sample-pair before searching backwards for the origin.
//
// (e.g. don't call from search then from playback then from search)
//
//Returns:
// FIRST_NIBBLE   a first (low) nibble has been received
// SECOND_NIBBLE  a second (high) nibble has been received
//                and was stored to *receivedCharacter
// INDICATOR      an indicator has been received
//                indicating that data exists somewhere within the last
//                minute (?)
// NADA           Nothing (should be) in the data-space...
//                (If there is, an error is printed)
//CALLED IN:
// main -> while(!quit)//playback -> for(z=0; z<numInSamples; z++)

//TESTING: This has been tested by allowing the program to think that the
//Header/UT-information is audio samples...
uint8_t parseDataFromSample(char * receivedCharacter, uint16_t sampleVal, 
                            uint8_t ignoreAlignmentErrors,
                            off_t sampleBytePosition)
{
   static uint8_t lastExtractedData = 0;
   uint8_t extractedData = (sampleVal & 0xfc00)>>8;
   //For easier viewing... Data's in the upper nibble, data-indicator in
   //the lower
   extractedData = (extractedData&0xf0) | ((extractedData&0x0c)>>2);

   //extractedData now contains both the (potential) data AND the
   //indication of which nibble and/or whether it's an indicator

   uint8_t dataIndication = extractedData & 0x03; //0x0c;
#define FIRST_NIBBLE 1//0x04
#define SECOND_NIBBLE 2//0x08
#define INDICATOR    3//0x0c
#define NADA         0 //0x00
   static uint8_t thisChar = 0;


   //Track which nibble has been received
   // 0 = none
   // 1 = first
   // 2 = second
   // 3 = WTF.
   static uint8_t lastReceivedNibbleNum = 0;

   //dataIndication is the indicator in the sample...
   switch(dataIndication)
   {
      case FIRST_NIBBLE:
         if(!ignoreAlignmentErrors)
            if(!((lastReceivedNibbleNum == 0) || (lastReceivedNibbleNum == 2)))
            {  
               printf("Data Skew!   @ ");
               
               printPosition(sampleBytePosition, TRUE);

               printf(
"  The sample's Data-Indicator says this sample contains data-nibble #1\n"
"  But audioThing[Desktop] seems to be expecting a value of 0 or 2\n"
"  (lastReceivedNibbleNum = %d)\n",  lastReceivedNibbleNum);
               printf(" Sample Data/Indicator: Last=0x%02x This=0x%02x\n",
                      lastExtractedData, extractedData);
            }
         lastReceivedNibbleNum = 1;
         //Clear the high nibble from the previous character
         thisChar = extractedData >> 4;
         break;
      case SECOND_NIBBLE:
         if(!ignoreAlignmentErrors)
            if(!(lastReceivedNibbleNum == 1))
            {
               printf("Data Skew!   @ ");
               
               printPosition(sampleBytePosition, TRUE);
               
               printf(
"  The sample's Data-Indicator says this sample contains data-nibble #2\n"
"  But audioThing[Desktop] seems to be expecting a value of 1\n"
"  (lastReceivedNibbleNum = %d)\n",  lastReceivedNibbleNum);
               printf(" Sample Data/Indicator: Last=0x%02x This=0x%02x\n",
                      lastExtractedData, extractedData);
            }
         lastReceivedNibbleNum++;
         thisChar |= (extractedData & 0xf0); 
         if(receivedCharacter != NULL)
            *receivedCharacter = thisChar;
         break;
      case INDICATOR:

         break;

      case NADA:
      default:
         if(extractedData & 0xf0)
         {
            printf("Data received without an indication of which nibble\n"
                   " and/or whether it's an indicator!  @ ");
            printPosition(sampleBytePosition, TRUE);
               printf(" Sample Data/Indicator: Last=0x%02x This=0x%02x\n",
                      lastExtractedData, extractedData);
         }
         break;
   }

   lastExtractedData = extractedData;

   return dataIndication;
}


//TODO: This could probably have been done with verbosity 0?
int searchReadable = FALSE;


int main(int argc, char *argv[])
{
    PaStreamParameters outputParameters;
    PaStream *stream;
    PaError err;
    float buffer[FRAMES_PER_BUFFER][2]; /* stereo output buffer */
    float sine[TABLE_SIZE]; /* sine wavetable */
    int left_phase = 0;
    int right_phase = 0;
    int left_inc = 1;
    int right_inc = 3; /* higher pitch so we can distinguish left and right. */
    int i, j; //, k;
   // int bufferCount;

// FILE *fid;

   int search = FALSE;
   int useUsageTable = TRUE;

   //If non-NULL we'll extract ...
   char *extractDir = NULL;
   
   int arg;

   char *fileString = NULL;
   
   //This is for basename() which may modify the original string
   // fileStringBase will point to a character in here, likely.
   char fileStringDirTemp[1000];
   char fileStringBaseTemp[1000];

   char *fileStringDir = NULL;
   char *fileStringBase = NULL;

   off_t limit_o = 0;
   off_t begin_o = 0;



// int sortedUTindices[USAGETABLE_SIZE+1] =
//                            { [0 ... USAGETABLE_SIZE] = -1};
// int sortUT_index = 0;


   for(arg = 1; arg<argc; arg++)
   {
      printf("arg=%d, argv[%d]='%s'\n", arg, arg, argv[arg]);
      if(argv[arg][0] == '-')
      {
         if(argv[arg][1] == 's')
         {
            search=TRUE;
            if(argv[arg][2] == 'r')
               searchReadable=TRUE;
         }
         else if((argv[arg][1] == 'u'))
         {
            //This is default anyhow...
            // Thus -u is redundant
            useUsageTable=TRUE;
         }
         else if((argv[arg][1] == 'n') && (argv[arg][2] == 'u'))
         {
            printf("Requested to *not* use the usage-table. "
                   "Functionality may be funky.\n");
            useUsageTable=FALSE;
         }
         else if(argv[arg][1] == 'e')
         {
            //extract = TRUE;
            //the argument is -e=<dir> so remove the '=' as well
            extractDir = &(argv[arg][3]);
            if(argv[arg][2] != '=')
            {
               printf("invalid argument\n");
               displayUsage();
               return 1;
            }
         }
         else if(argv[arg][1] == 'V')
         {
            printf("volume...\n");
            if((1 != sscanf(argv[arg], "-V=%f", &volume)))
            {
               printf("invalid argument '%s'\n", argv[arg]);
               displayUsage();
               return 1;
            }
         }
         else if(argv[arg][1] == 'v')
         {
            if(1 != sscanf(argv[arg], "-v=%d", &verbosity))
            {
               printf("invalid argument: '%s'\n", argv[arg]);
               displayUsage();
               return 1;
            }
         }
         else if(argv[arg][1] == 'h')
         {
            displayUsage();
            return 1;
         }
         else if((argv[arg][1] == 'l') || (argv[arg][1] == 'b'))
         {
            off_t *p_toWrite_o;
            if((argv[arg][1] == 'l'))
            {
               printf("Limiting search to ");
               p_toWrite_o = &limit_o;
            }
            else if ((argv[arg][1] == 'b'))
            {
               printf("Starting at ");
               p_toWrite_o = &begin_o;
            }
            else
            {
               printf("WTF, we shouldn't get here\n");
               // But gcc's not recognizing this
               //  claiming that p_toWrite_o might be used uninitialized
               return 1;
            }
/*
            //handle limited-search...
            char character = 'b';

            //Byte Offset (in hex)
            int scanRet = sscanf(&(argv[arg][2]), "=0x%"SCNx64, 
                                                   p_toWrite_o);

            //Byte offset in decimal b/k
            if(scanRet != 1)
               scanRet = sscanf(&(argv[arg][2]), "=%"SCNu64"%c",
                                             p_toWrite_o,   &character);

            if(character == 'k')
               (*p_toWrite_o) *= 1024;
            else if((character != 'b') || (scanRet < 1))
            {
               printf("Invalid Argument\n");
               return 1;
            }
*/

            *p_toWrite_o = scanPosition(&(argv[arg][3]));

            //Make sure it was valid... MUST have "-[l/b]="
            if((argv[arg][2] != '=') || (*p_toWrite_o == SCAN_UNHANDLED))
            {
               printf("Invalid Argument\n");
               displayUsage();
               return 1;
            }


            printf("byte position ");// 0x%"PRIx64
                 //"=%"PRIu64"\n", *p_toWrite_o, *p_toWrite_o);
            printPosition(*p_toWrite_o, TRUE);
         }
      }
      else if(fileString == NULL) 
         //Non-dash arg... only one so far, the file...
      {
         fileString = argv[arg];
         //basename():
         //"The function may modify the string pointed to by path."
         strncpy(fileStringDirTemp, fileString, 1000);
         if(fileStringDirTemp[999] != '\0')
         {
            printf("file name/path is too long\n");
            return 1;
         }

         fileStringDir = dirname(fileStringDirTemp);
         
         printf("dir: '%s'\n", fileStringDir);
         
         strncpy(fileStringBaseTemp, fileString, 1000);
         if(fileStringBaseTemp[999] != '\0')
         {
            printf("file name/path is too long\n");
            return 1;
         }

         fileStringBase = basename(fileStringBaseTemp);
         printf("file: '%s'\n", fileStringBase);

      }
      else
      {
         displayUsage();
         printf("Too many/Invalid arguments\n");
         return 1;
      }
   }
/*
   if(argc == 1)
   {
      char fileString[] = "/dev/disk3"; ///Volumes/audioTest/recorded.raw";
      printf("Using Default Input File: /dev/disk3.");
      fid = fopen(fileString, "rb");
   }
*/



   if(fileString == NULL)
   {
      printf("need a file! e.g. 'audioThing /dev/disk4'\n");
      displayUsage();
      return 1;
   }
   else
   {
#if(defined(__MYDEF_MAC_OS_X__) && __MYDEF_MAC_OS_X__)
      if(search)
      {
         if(0==strncmp(fileStringBase,"rdisk",5))
         {
            printf("Attempting to search /dev/rdisk#\n"
               "  That's not really possible. Use /dev/disk# instead.\n"
               "  (Note that searching /dev/disk# is SLOW\n"
               "   So might be wise to extract /dev/rdisk# first,\n"
               "   then search)\n");
            return 1;
         }
         else if(0==strncmp(fileStringBase,"disk",4))
         {
            printf("Attempting to search /dev/disk#\n"
                   "  (Note that searching /dev/disk# is SLOW\n"
               "   So might be wise to extract /dev/rdisk# first,\n"
                   "   then search)\n");
         }
      }
#endif


      printf("Input File: '%s'\n", fileString);
      fid = fopen(fileString, "rb");



      //Attempt to determine the file-size
      //In MacOS, if your file is a disk-device: e.g. /dev/disk3 (the
      // SD-Card, unmounted, since there's no file-system)
      //Then this test will return 0 and we'll enter the if(fileSize == 0),
      // below.
      //Similar for linux, see below.

       fseeko(fid, 0, SEEK_END);

       //Must include byte 0...
       // Apparently it does that... because +1 leads to odd-byte seeking
       fileSize = ftello(fid);





      //TODO:
      // Maybe a more reasonable test would be to check if the file is
      // located in /dev/ first...?

#if(defined(__linux) && __linux)

      //This doesn't enter as expected, apparently Debian doesn't mind
      //seeking to the end of /dev/sdn even though it's not been mounted...
      // (This differs from MacOS)
      //SO: This code is untested.
      if( fileSize == -1 )
      {
         printf("Filesize couldn't be determined using fseek()\n");
         

         // (BLKGETSIZE in linux... per:
         //  http://www.linuxproblem.org/art_20.html )
         unsigned long blockCount;
         ioctl(fileno(fid), BLKGETSIZE, &blockCount);
         printf("BlockCount: %lu\n", blockCount);
         //The example there assumes blocksize is 512B
         // But, e.g., the 2GB SD Cards that are NOT HighCapacity
         // get away with such a large value because their block size is
         // 1024B... So, does Linux know this?
         //uint32_t blockSize;
         //ioctl(fileno(fid), DKIOCGETBLOCKSIZE, &blockSize);
         //printf("BlockSize: %"PRIu32"\n", blockSize);
         printf(
         "NOTE: ASSUMING the size of the above-mentioned blocks is 512B.\n"
         "(Might be a fact of linux's 'BLKGETSIZE'\n"
         " regardless of the device's internal blocksize)\n");

         fileSize = (off_t)blockCount*(off_t)512; //blockSize;

      }
#endif


      //Not sure this is 100% safe...
      // but it appears that MacOS would return 0 with this method
      // for /dev/disk# devices...
      if(fileSize == 0)
      {
         printf("Filesize couldn't be determined using fseek()\n");

#if(defined(__linux) && __linux)
         return 1;
#endif


#if(defined(__MYDEF_MAC_OS_X__) && __MYDEF_MAC_OS_X__)
         /*
         // Doesn't help, either.
         struct stat fileStats;

         //fstat( //fstat(int fd... as opposed to FILE * fd... WTF?
         //stat(fileString, &fileStats);

         perror("stat:");
         printf("Stat File Size: %"PRIu64"\n", fileStats.st_blocks);
         */

         //see: /usr/include/sys/disk.h (MAC OS)
         // (BLKGETSIZE in linux... per:
         //  http://www.linuxproblem.org/art_20.html )
         uint64_t blockCount;
         ioctl(fileno(fid), DKIOCGETBLOCKCOUNT, &blockCount);
         printf("BlockCount: %"PRIu64"\n", blockCount);
         uint32_t blockSize;
         ioctl(fileno(fid), DKIOCGETBLOCKSIZE, &blockSize);
         printf("BlockSize: %"PRIu32"\n", blockSize);

         fileSize = blockCount*blockSize;

         //printf("Capacity: %f KB\n", 
         // (float)(blockCount)*(float)(blockSize)/(float)(1024));
#endif
      }


      //This per:
      //http://stackoverflow.com/questions/586928/how-should-i-print-types-like-off-t-and-size-t
      printf("File Size: %ju Bytes\n", (uintmax_t)fileSize);
      

      // file_numBlocks = fileSize/512
      // numBlocksPerChunk = file_numBlocks/256 (UT entries)
      //numBlocksPerChunk = (fileSize/512)>>8;

      //printf(" Per Chunk: ");
      //printPosition(numBlocksPerChunk*512, TRUE);

      fseeko(fid, 0, SEEK_SET);
   }
   
   if( fid == NULL )
   {
      printf("Unable to open file\n");
      return 1; //exit(1);
   }

   #define FORMATHEADER_SIZE  256
   //#define USAGETABLE_SIZE  256
   //Store the usage-table for easy lookup...
   //uint8_t ut[USAGETABLE_SIZE];


   int headerType = loadHeader();

   //Skip playback and/or searching of *the header data*  in auT and auTx 
   // files
   //(An alternative might be, e.g., raw data extracted from the card 
   // via 'dd'... there've been cases, e.g. where the formatting was
   // corrupted)
//#define TEST_OFFSET_ERROR TRUE
#if( !defined(TEST_OFFSET_ERROR) || !TEST_OFFSET_ERROR)
   if( (    (headerType == auTx_HEADER)
         || (headerType == auT_HEADER) 
       )
       && (begin_o == 0))
   {
      begin_o = HEADER_SIZE;
   }
#else
 #warning "!!!! TEST_OFFSET_ERROR IS TRUE !!!! This is only for debugging!"
#endif

   //int loopMax = 0;

   if(useUsageTable)
   {
      //UT_HEADER == auT_Header
      //(There's no usage-table in an auTx (extracted) file, as each
      //individual auTx file is a single entry in the original usage-table
      if((headerType != UT_HEADER))
      {
         printf("UsageTable requested, but this file does not have one.\n"
                " Skipping...\n");

         useUsageTable = FALSE;
      }
      else
      {
         // file_numBlocks = fileSize/512
         // numBlocksPerChunk = file_numBlocks/256 (UT entries)
         numBlocksPerChunk = (fileSize/512)>>8;
         printf(" Per Chunk: ");
         printPosition(numBlocksPerChunk*512, TRUE);


         int retVal;

         //Load the UsageTable
         retVal = loadUT(); //&loopMax);
         if(retVal)
            return retVal;
      
         //Sort the UsageTable by loopNum... 
         // Also fills FragList2
         retVal = sortUT(); //loopMax);
         if(retVal)
            return retVal;


         printFragList();
      }
   }



   //Since seeking is done in the search-loop, we'll save it for non-search
   // until after search...
 

   //the KB indicator lasts for 4507 512B blocks...
   //(see audioThing[AVR]/mainConfig.h)
   // NOT SURE how this works out with chunks...
   //  will vary depending on the card-size (?)
   #define SEEK_AMOUNT  (4506ll*512ll)

   if(search)
      searchForText(useUsageTable, begin_o, limit_o);

   if(extractDir != NULL)
   {
      if(!useUsageTable)
      {
         printf("extraction requires -u\n");
         return 1;
      }

      int fragListIndex;

      off_t totalBytesToCopy = 0;
      //Calculate the total number of bytes to copy
      for(fragListIndex = 0; fragList2[fragListIndex].firstChunk != -1;
            fragListIndex++)
      {
         off_t fragStartByte;
         off_t fragEndByte;

         getFragBytes(fragListIndex, &fragStartByte, &fragEndByte);
         totalBytesToCopy += (fragEndByte - fragStartByte + 1);

      }


      struct timeval extractionStartTime;
      gettimeofday(&extractionStartTime, NULL);

      off_t totalBytesCopied = 0;

      for(fragListIndex = 0; fragList2[fragListIndex].firstChunk != -1;
         fragListIndex++)
      {
         FILE *outFile;
      
#define BLOCKSIZE 512
         uint8_t block[BLOCKSIZE];

         char outFileString[1000];

         char memoStartTimeString[20];
         char memoEndTimeString[20];
/*
         off_t startByte;
         off_t endByte;

         getFragBytes(fragListIndex, &startByte, &endByte);
*/
         //This is for *all* linked fragments...
         timeStringsFromFrag(fragListIndex, memoStartTimeString, 
                              memoEndTimeString, TRUE);

         snprintf(outFileString, 1000, "%s/Boot%d_%s-%s.auTx", extractDir,
               (fragList2[fragListIndex].loopNum)>>4, 
               memoStartTimeString, memoEndTimeString);

         printf("Output File: '%s'\n", outFileString);

         // "'b' is ignored" "strictly for compatibility with ISO C90"
         // 'x' is an extension which will return an error if the
         //     file already exists... it may not be portable.
         //     would be nice, though.
#warning "NEED TO ADD 'x' TO FOPEN!!! or otherwise test whether the file exists before overwriting when extracting..."
         outFile = fopen(outFileString, "wb"); //x");

         if(outFile == NULL)
         {
            perror("Couldn't create file: ");
            return 1;
         }

//#error "This needs to be moved..."
/*
off_t startByte;
off_t endByte;
getFragBytes(fragListIndex, &startByte, &endByte);

//From here was original...

         //Seek to the start-byte...
         int seekResult = fseeko(fid, startByte, SEEK_SET);

         if(seekResult != 0)
         {
            perror("Couldn't seek input file: ");
            return 1;
         }

         printFragInfo(fragListIndex, TRUE, TRUE, TRUE, TRUE);
         off_t bytesGotten = 0;
         off_t bytesTotal = endByte - startByte + 1;
*/
         printf("Writing Header:\n");
         
         for(i=0; i<256-8; i++)
         {
            uint8_t formattingIndicatorByte = 255-i;
            
           int byteWritten = 
            fwrite(&formattingIndicatorByte, sizeof(uint8_t), 1, outFile);
            if(byteWritten != 1)
            {
               printf("Unable to write header\n");
               perror(NULL);
               return 1;
            }
         }

         char sampleRateString[8] = { [0 ... 7] = '\0' };
         snprintf(sampleRateString, 8, "%d%c", sampleRate, 
                              ((srAssumed) ? '?' : '\0'));

         int bytesWritten =
            fwrite(sampleRateString, sizeof(uint8_t), 8, outFile);
         if(bytesWritten != 8)
         {
               printf("Unable to write header\n");
               perror(NULL);
               return 1;
         }

         char newHeaderData[256] = { [0 ... 255] = '\0' };

         time_t now;
         time(&now);
         char extractionTimeString[30];

         strncpy(extractionTimeString, ctime(&now), 30);

         time_t modTime;
         struct stat stats;
         //Another useful stat here: 
         // st_blksize: /* optimal file sys I/O ops blocksize */

         stat(fileString, &stats);
         modTime = stats.st_mtime;
         char modTimeString[30];
         strncpy(modTimeString, ctime(&modTime), 30);


         if(0 == strncmp(fileStringDir, "/dev", 4))
            snprintf(newHeaderData, 256, 
                  "Extracted Directly From Card: %s\n"
                  "At: %s\n", fileString, extractionTimeString);
         else
            snprintf(newHeaderData, 256, 
               "Original File: '%s'\n"
               "Last Modified: %s"
               "Extracted At: %s\n",
               fileStringBase, modTimeString, extractionTimeString);


         char moreHeaderData[256];

         snprintf(moreHeaderData, 256, "Boot Num: %d\n"
                                       "Time Since Boot: %s -> %s\n"
                                       "Sample Rate%s: %dS/s\n",
               (int)((fragList2[fragListIndex].loopNum)>>4),
               memoStartTimeString, memoEndTimeString,
               (srAssumed ? "(Assumed)" : ""), sampleRate);

         strncat(newHeaderData, moreHeaderData, 255-strlen(newHeaderData));

         printf("%s", newHeaderData);

         if(fwrite(newHeaderData, sizeof(uint8_t), 256, outFile) != 256)
         {
            printf("Unable to write header info\n");
            perror(NULL);
            return 1;
         }
         
         //Now the header-stuff is out of the way, we can copy the data.

         while(1)
         {
            off_t fragStartByte;
            off_t fragEndByte;
            getFragBytes(fragListIndex, &fragStartByte, &fragEndByte);
            //Seek to the start-byte...
            int seekResult = fseeko(fid, fragStartByte, SEEK_SET);
            if(seekResult != 0)
            {
               perror("Couldn't seek input file: ");
               return 1;
            }
            printFragInfo(fragListIndex, TRUE, TRUE, TRUE, TRUE);
            off_t bytesGotten = 0;
            off_t bytesTotal = fragEndByte - fragStartByte + 1;




            //Print the total progress bar...
            printf("Overall:\n");
            printProgressBar(totalBytesCopied, totalBytesToCopy); 
            printTimeEstimate(totalBytesCopied, totalBytesToCopy,
                                                   extractionStartTime);

            printf("\n Bytes to copy (this fragment): %"PRIu64"\n", bytesTotal);

            off_t bytesGottenPriorToScrollerUpdate = 0;

            struct timeval fragExtractStartTime;
            gettimeofday(&fragExtractStartTime, NULL);


            while(bytesGotten < bytesTotal)
            {
               off_t bytesToGet = (bytesTotal - bytesGotten > BLOCKSIZE) ?
                  BLOCKSIZE : (bytesTotal - bytesGotten);

               if(bytesToGet != BLOCKSIZE)
                  printf("BytesToGet=%"PRIu64" != BLOCKSIZE\n", bytesToGet);

               int numBytes =
                  fread(block, sizeof(uint8_t), bytesToGet, fid);

               if(numBytes <= 0)
               {
                  printf("End Of Input File\n");
                  perror(NULL);
                  break;
               }


               bytesGotten += numBytes;

               int bytesOut = 
                  fwrite(block, sizeof(uint8_t), numBytes, outFile);
   
               if(bytesOut != numBytes)
               {
                  printf("Couldn't write all the input bytes!");
                  perror(NULL);
                  break;
               }


               if(bytesGotten - bytesGottenPriorToScrollerUpdate > 1024*512)
               {
                  bytesGottenPriorToScrollerUpdate = bytesGotten;


                  printProgressBar(bytesGotten, bytesTotal);

                  printTimeEstimate(bytesGotten, bytesTotal,
                                                fragExtractStartTime);
               }
            }  

            printf("\n");
            //bytesTotal is in *this fragment*
            totalBytesCopied += bytesTotal;

            if(!fragList2[fragListIndex].linkedWithNext)
               break;

            fragListIndex++;
         }
         
         if(fclose(outFile) != 0)
         {
            perror("Couldn't close file: ");
            return 1;
         }
      }

      return 0;
   }







   off_t now_o = begin_o;

   int seekResult = fseeko(fid, begin_o, SEEK_SET);

   if((seekResult != 0) || errno)
   {  
         printf("fseeko failed...\n");
         perror(NULL);
            return 1;
   }




    printf("PortAudio Output: SampleRate = %d, BufSize = %d\n", 
          SAMPLE_RATE, FRAMES_PER_BUFFER);
    


    errno_handleError("prior to PortAudio init", ERRHANDLE_DONT_RETURN);

    err = Pa_Initialize();
    errno_handleError("after PortAudio init", ERRHANDLE_DONT_RETURN);

    if( err != paNoError ) goto error;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */

    errno_handleError("after Pa_GetDefaultOutputDevice()", ERRHANDLE_DONT_RETURN);
    
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    errno_handleError("after Pa_GetDeviceInfo()", ERRHANDLE_DONT_RETURN);

    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              NULL, /* no input */
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,      /* we won't output out of range samples so don't bother clipping them */
              NULL, /* no callback, use blocking API */
              NULL ); /* no callback, so no callback userData */
    if( err != paNoError ) goto error;

    errno_handleError("after Pa_OpenStream", ERRHANDLE_DONT_RETURN);


    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;
    errno_handleError("after Pa_StartStream", ERRHANDLE_DONT_RETURN);


    //This is a remnant of the original PortAudio example-code, and can
    //probably be removed...
    for( j=0; j < FRAMES_PER_BUFFER; j++ )
   {
       buffer[j][0] = sine[left_phase];  /* left */
       buffer[j][1] = sine[right_phase];  /* right */
       left_phase += left_inc;
       if( left_phase >= TABLE_SIZE ) left_phase -= TABLE_SIZE;
       right_phase += right_inc;
       if( right_phase >= TABLE_SIZE ) right_phase -= TABLE_SIZE;
   }

   #define OUT_SAMPLES_PER_IN_SAMPLE \
            ((float)(SAMPLE_RATE) / (float)(sampleRate))

   #define IN_SAMPLES_PER_OUT_SAMPLE \
            ((float)(sampleRate) / (float)(SAMPLE_RATE))

   #define IN_FRAMES_PER_BUFFER \
            ((IN_SAMPLES_PER_OUT_SAMPLE) * (float)(FRAMES_PER_BUFFER))

   #define MAX_SPEED 10

   #define BUFFER_1CH16B_SIZE (FRAMES_PER_BUFFER*MAX_SPEED)

   uint16_t buffer_1ch16b[BUFFER_1CH16B_SIZE];
                     //a/o v7p13 this was FRAMES_PER_BUFFER*10
                     //    I'm *assuming* 10 was supposed to be MAX_SPEED

   //lastPrinted_o is only used for updating the timer once per second
   // off_t is signed, so putting a large negative value here will assure
   // that it displays *immediately* when starting (rather'n waiting for a
   // full second before displaying)
   off_t lastPrinted_o = -(sampleRate*2)*2;
   uint8_t dataPrinted = FALSE;

   printf("Beginning Playback... q-<enter> to quit\n");
   
   
   
   
   
   
   
   
    errno_handleError("prior to stdinNB_init()", ERRHANDLE_DONT_RETURN);
   
   if(stdinNB_init())
   {
      printf("stdinNB_init() failed\n");
      return 1;
   }


   int quit = 0;
   char inString[100];
   int stringIndex=-1;
   float speed = 1.0;

   //If the "indicator" is active, show it...
   // This is used for searching, generally, so to search for text-input,
   // we can instead search in huge jumps for only the indicator...
   // It doesn't seem to be working with the SDHC, so I want to see it
   // during playback.
   uint8_t indicator = FALSE;
   uint8_t lastIndicator = FALSE;


   //For debugging of audioThingAVR v50+'s audio quality issues...
   // let's compare data from the new version vs the old ATtiny861
   int16_t sampleMax = INT16_MIN;
   int16_t sampleMin = INT16_MAX;
   
   //To explore volume cutting-out at 10
   static float minBuff = 100;
   static float maxBuff = -100;


   while(!quit)
   {
    errno_findError("1", ERRHANDLE_DONT_RETURN);
      int numSamplesToGrab = (int)((float)IN_FRAMES_PER_BUFFER*speed);

      int numInSamples = //FRAMES_PER_BUFFER;
           freadSamples(buffer_1ch16b, numSamplesToGrab, fid);
//         fread(buffer_1ch16b, sizeof(uint16_t), numSamplesToGrab, fid);

      //now_o will be updated at the end of the loop, as it's used for
      //printout, etc.

    errno_findError("2", ERRHANDLE_DONT_RETURN);





      int kbChar = stdinNB_getChar();
    errno_findError("3", ERRHANDLE_DONT_RETURN);


      if(kbChar != -1)
      {
         printf("stdin: '%c'\n", (char)kbChar);
         inString[++stringIndex]=kbChar;
         inString[stringIndex+1]='\0';
      }
      else if(stringIndex >= 0)
      {
         if(inString[0] == 'h')
         {
            displayPlaybackUsage();
            stringIndex=-1;
         }
         else if(1==sscanf(inString, "v=%f", &volume))
         {
            printf("Volume=%f\n", volume);
            stringIndex=-1;
         }
         else if(1==sscanf(inString, "s=%f", &speed))
         {
            if(speed < 0.1)
               speed = 0.1;
            if(speed > MAX_SPEED)
               speed = MAX_SPEED;

            printf("Speed=%f\n", speed);
            stringIndex=-1;
         }
         //else if (0==strncmp(inString, "s++", 3))
         //{
         // stringIndex=-1;
         // speed++;
         //}
         //else if (0==strncmp(inString, "s--", 3))
         //{
         // stringIndex=-1;
         // speed--;
         // if(speed <= 0.25)
         //    speed=0.25;
         //}
         else if (inString[0] == 'q')
         {
            printf("Quitting...\n");
            quit=1;
         }
         else
         {
            printf("Unhandled Command: '%s'\n", inString);
            stringIndex=-1;
         }
         /*
         switch(inString[stringIndex--])
         {
            case -1: //Nothing Received...
               break;
            case 'q':
               quit = 1;
               break;
            default:
               printf("Received: '%c'\n", (unsigned char)kbChar);
         } 
         */
      }

    errno_findError("4", ERRHANDLE_DONT_RETURN);
      static int loopCount;

      loopCount++;
      
      if(now_o - lastPrinted_o > sampleRate*2)
      {
         static uint8_t lastPrintedChars = 0;
         lastPrinted_o = now_o;

      
         //uint64_t secTotal = (now_o / (sampleRate*2));

         //uint64_t hours = secTotal / (60*60);
         //uint64_t minutes = (secTotal - (hours*60*60))/60;
         //uint64_t secs = secTotal - hours*60*60 - minutes*60;

         dataPrinted = 0;

         //can't '\r' beforehand, 'cause stdin is printing at the last
         //position, which means after a bytePosition update, it restarts
         //at the end of -- bytePosition... --
         // (overwriting itself)
         lastPrintedChars = 
            printf("   --- bytePosition: "); //0x%"PRIx64" = %"PRIu64"s = %"
                  //PRIu64"h%"PRIu64"m%"PRIu64"s---",
                  //now_o, secTotal, hours, minutes, secs);
         lastPrintedChars += 
            printPosition(now_o, FALSE);

         //for( ; lastPrintedChars < 60; )
         // lastPrintedChars +=
         //    printf(" ");

         if(indicator)
         {
            lastPrintedChars +=
               printf(" <I>");

            //It will be reset again in the next sample.
            indicator=FALSE;
            lastIndicator=TRUE;
         }
         else if (lastIndicator) // && !indicator)
         {
            lastPrintedChars +=
               printf(" <I>");

            //Cause the indicated-line to be preserved
            // after indication stops...
            dataPrinted=TRUE;
            lastIndicator=FALSE;
         }



         lastPrintedChars +=
            printf(" loopCount=%d", loopCount);
         loopCount = 0;

         lastPrintedChars +=
            printf(" range=[%"PRIi16",%"PRIi16"]=%"PRIi16,
                      sampleMin, sampleMax, (sampleMax - sampleMin));
         sampleMin = INT16_MAX;
         sampleMax = INT16_MIN;


#if(defined(PRINT_VOLUME) && PRINT_VOLUME)
         lastPrintedChars +=
            printf(" buff:[%f,%f]=%f", minBuff, maxBuff,
                  (maxBuff-minBuff));
         minBuff=100;
         maxBuff=-100;
#endif

         lastPrintedChars +=
            printf(" ---");

//#define SUPPRESS_BACKSPACES TRUE

#if(!defined(SUPPRESS_BACKSPACES) || !SUPPRESS_BACKSPACES)
         //No idea what this is about, exactly... but it's used later
         // (dataPrinted)
         if(!dataPrinted)
         {
            //Would it be better to just use '\r'?
            // NOT using '\r' allows for user-input at the beginning of the
            // line...
            uint8_t i;
            for(i=0; i<lastPrintedChars; i++)
               printf("\x08");
         }
         else
            printf("\n");
#endif
         fflush(stdout);
      }
     
    errno_findError("5", ERRHANDLE_DONT_RETURN);

      //if((int)numInSamples != (int)IN_FRAMES_PER_BUFFER) //0)
      if((int)numInSamples != (int)numSamplesToGrab) //0)
      {
         printf("read %d input samples, expected %d. (at %dS/s)\n", 
         //numInSamples, (int)IN_FRAMES_PER_BUFFER, (int)sampleRate);
         numInSamples, (int)numSamplesToGrab, (int)sampleRate);
         break;
      }


    errno_findError("6", ERRHANDLE_DONT_RETURN);
//    int numOutSamples = (float)numInSamples * OUT_SAMPLES_PER_IN_SAMPLE; 
      int numOutSamples = (float)IN_FRAMES_PER_BUFFER
                          * OUT_SAMPLES_PER_IN_SAMPLE;

      int z;
      //uint8_t character = 0;
      char character = 0;
//    uint8_t receivedNibble = 0;
      //Extract the data
      // Have to go through every sample, regardless of whether it'll be
      // fed to the output (e.g. if fast-forwarding)
      for(z=0; z<numInSamples; z++)
      {
         //uint8_t receivedNibble = 0;
//#warning "parseDataFromSample has not been implemented in playback"
         //uint8_t extractedData = ((buffer_1ch16b[z] & 0xfc00)>>8);
      
         uint8_t dataType = 
            parseDataFromSample(&character, buffer_1ch16b[z], 
                                FALSE, 
                                now_o + (z * BYTES_PER_SAMPLE)); 

         if(dataType == SECOND_NIBBLE)
         {
            if(!dataPrinted)
               printf("\n");

            dataPrinted = TRUE;
            printf("%c", character);
            if(character == '\r')
               printf("\n");
            else if(character == '\n')
               printf("\r");
            fflush(stdout);
         }
         else if (dataType == INDICATOR)
            indicator=TRUE;

         //Leave with the buffer containing only sample data
         buffer_1ch16b[z] &= 0x3ff;


         //Min/Max for printout...
         if( buffer_1ch16b[z] > sampleMax )
            sampleMax = buffer_1ch16b[z];
         if( buffer_1ch16b[z] < sampleMin )
            sampleMin = buffer_1ch16b[z];
        
         
         sampleOffset = sampleOffset 
                        + ((float)buffer_1ch16b[z] - sampleOffset) 
                          / offsetDelay;   

      }

    errno_findError("7", ERRHANDLE_DONT_RETURN);

      
      for(z=0; z<numOutSamples; z++)
      {
         int iz = (float)z * IN_SAMPLES_PER_OUT_SAMPLE * speed;
         if(iz >= numInSamples)
            iz=numInSamples-1;
         //static int lastiz = 0;
         //static uint8_t character = 0;


         //This attempts to center samples at 0 and adjust for volume
         buffer[z][0] = 
            ((float)buffer_1ch16b[iz]-sampleOffset)*volume/512.;

         //a/o v7p15: volume > 10 results, ultimately, in near silence
         // (all samples out-of-range?)
         //(buffer is float, min=-1 (or 0?), max=1)
         //BUT: apparently, at least in linux, I'm getting silence if the
         //total volume, including that of the sound control-panel, is
         //greater than 1...
         //I *think* what's happening is something to do with the values...
         // e.g. Min/Max sample-values when needing to amplify this much
         // has a range of only around 7... yes, that's 7 out of 1024
         // Further, the offset isn't centered around 512
         // So, with so much amplification, this results in values below
         // the sound-card's minimum... rather than values clipping at
         // *both* the maximum and minimum.
         // I need to explore this further (and maybe have a variable
         // offset)
         if(buffer[z][0] > maxBuff)
            maxBuff = buffer[z][0];
         if(buffer[z][0] < minBuff)
            minBuff = buffer[z][0];


         //buffer[z][1] = ((float)buffer_1ch16b[iz]-512.)*volume/512.;
         buffer[z][1] = buffer[z][0];
      }

    errno_findError("8", ERRHANDLE_DONT_RETURN);
      err = Pa_WriteStream( stream, buffer, numOutSamples); //numBytes); 
      //FRAMES_PER_BUFFER );
      if( err != paNoError ) goto error;

    errno_findError("9", ERRHANDLE_DONT_RETURN);
      //All This Debugging (errno_findError() calls)
      // to determine that Pa_WriteStream() is returning with errno set
      // "Resource Temporarily Unavailable"
      // same as stdinNB_getChar() has to clear each time there's no data
      // available... So, it interfered
#if(defined(__MYDEF_MAC_OS_X__) && __MYDEF_MAC_OS_X__)
      //Actually, on OSX, I never had this problem... so running this test
      //and the associated code is *untested* and probably unnecessary.
      if(errno == 35) //see notes of similar usage in stdin_nonblock.h
#elif(defined(EAGAIN))
      if(errno == EAGAIN)
#else
 #error "Not sure what your system'll do, you might just get away with removing this error"
      if(0)
#endif
      {
         errno = 0;
      }


      

      now_o += (numInSamples*BYTES_PER_SAMPLE);

   }   


        err = Pa_StopStream( stream );
        if( err != paNoError ) goto error;

        ++left_inc;
        ++right_inc;

        Pa_Sleep( 1000 );
   

    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    Pa_Terminate();
    printf("Test finished.\n");
    
    return err;
error:
    Pa_Terminate();
    fprintf( stderr, "\n");
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    return err;
}





//Search:
void searchForText(int useUsageTable, off_t searchBegin_o, off_t searchLimit_o)
   {

      v_printf(1,"Beginning Search...\n");

      off_t seek_o;
      off_t soughtAmount_o = 0; //SEEK_AMOUNT;

      //begin_o is overridden if useUsageTable is true
#warning "begin_o needs to be revisited, for resumed-searches"
      // it will start at the first marked chunk in the lowest loopNum,
      // regardless of begin_o.
      seek_o = searchBegin_o;
      
      int researching = FALSE;
      int indicatorCount = 0;

      //Used later for determining the time of a sample within this
      // "session" since boot (for printout)
      // ONLY BY printPosition2
      int thisStartChunk = 0;

      //off_t thisStartByte=0;
      // Tells the search when to quit searching...
      // (end of file, end of fragment, etc.)
      off_t thisEndByte=0;

      //NEW This is used internal to the usage-table start/end 
      //  determination
      // After the usage-table portion of the do-while loop, its value
      //  is the _next_ fragListIndex to be processed.
      // But also used to determine when the do-while-loop has completed
      int fragListIndex = 0;

      do
      {
         if(searchReadable)
            printf("########################################\n");
         if(useUsageTable)
         {
            //Used later for determining the time of a sample...  
            thisStartChunk = fragList2[fragListIndex].firstChunk;


            //This should only be the case if the fragment-list is empty
            // otherwise the do-while will handle the end of the 
            // fragment-list

            if(fragList2[fragListIndex].firstChunk == -1) 
            {
               printf("End of Fragment-List found\n");
               exit(0);
            }

            
            printFragInfo(fragListIndex, TRUE, TRUE, TRUE, TRUE);
         
            getFragBytes(fragListIndex, &seek_o, &thisEndByte);

            //Prep the next loop...
            //This shouldn't be used again until we re-enter this 'if'
            // EXCEPT for checking whether the do-while is complete
            fragListIndex++;
         }
         else if(searchLimit_o)
         {
            thisEndByte = searchLimit_o;
         }
         else
         {
            //WAS: fileSize, but fileSize is the NUMBER OF BYTES (right?)
            thisEndByte = fileSize-1;
         }

      // When this is TRUE, SEEK will be set to the 
      //    limit/endOfChunk/endOfFile
      // rather than attempting to seek past the end
      // but, upon doing-so, this will be set FALSE
      // at which point, the next seek past the end will
      // cause the loop to break

      int endTest = TRUE;
      while(1)
      {
         if(seek_o & 0x01)
         {
            printf("seeking an odd-byte! WTF?\n");
            exit(1);
         }
         //This should be handled at the end, now...
      /*
         //If we've reached the end of the chunk, break to get the next
         //  marked chunk and start again.
         if(useUsageTable && (seek_o > thisEndByte) )
         {
            if(endTest)
            {
               //-1 because it grabs two at a time for a sample...
               seek_o = thisEndByte-1;
               endTest = FALSE;
            }
            else
               break;
         }
      */
         int seekResult = fseeko(fid, seek_o, SEEK_SET);

         if((seekResult != 0) || errno)
         {
            printf("fseeko failed...\n");
            perror(NULL);
            exit(1);
         }

         //Since everything's even, we should be able to grab directly...
         // without worrying about grabbing the low-byte by accident...

         uint16_t sample;
         //Grab 1 sample...
         //Should be 1 unless we've reached the EOF...
//#warning "fread(...(uint16)...) isn't portable!"
         int numInSamples =
              freadSamples(&sample, 1, fid);
//            fread(&sample, sizeof(uint16_t), 1, fid);

         if(numInSamples <= 0)
         {
            printf("End Of File reached...\n"
                   "  This should be smarter than that!\n"
                   "  Did you attempt to search /dev/rdisk#\n"
                   "  instead of /dev/disk#?\n"
                   "  (/dev/rdisk can only be accessed per-sector(?))\n");
            exit(1);
         }


            



         //uint8_t extractedData = ((sample & 0xfc00)>>8);


         //May be overwritten below
         // and since it's static '=0' is irrelevent.
         //static uint8_t thisChar = 0;
         //It should no longer need to be static, right?
         // It's only used when the second nibble is received, RIGHT?
         char thisChar = 0;

         uint8_t dataType =
            parseDataFromSample(&thisChar, sample, 
                                 !researching, seek_o); //FALSE);
         
         if(researching)
         {
            if(dataType == FIRST_NIBBLE)
            {
               researching++;
               indicatorCount = 0;
            }
            else if (dataType == SECOND_NIBBLE)
            {
                  //thisChar has been set
                  v_check(0)
                  {

                     if(searchReadable)
                     {
                        printf("%c", thisChar);
                        fflush(stdout);
                     }
                     else
                     {
                        if(isprint(thisChar))
                           printf("Found '%c'=0x%02x  @ ", 
                              thisChar, (int)(thisChar));
                              //0x%"PRIx64"\n", thisChar, seek_o);
                        else
                           printf("Found    <0x%02x> @ ", (int)(thisChar));


                        printPosition2(useUsageTable, thisStartChunk, 
                                       seek_o, FALSE);
                     }
                  }
                  else
                  {
                     if(isprint(thisChar))
                        printf("%c", thisChar);
                     else
                        printf("<0x%02x>", (int)(thisChar));
                  }

                  if(thisChar == HSSKB_MEMO_RETURN)
                     printf(" <HSSKB Memo-Key>");
                  else if(thisChar == NKP_MEMO_RETURN)
                     printf(" <NKP Memo-Key>");
               
                  v_check(0)
                  {
                     if(!searchReadable)
                        printf("\n");
                  }
                  researching++;
                  indicatorCount = 0;
            }
            //Data already found...
            else if(researching > 1)
            {
         /*
            //This might help speed up search time...
            // once the data's been read-back, we could skip ahead another
            // search-frame and see if we need to come back
            else if(((extractedData & 0x0c) == 0x0c) && (researching > 1))
            {
               //This is where things start to get a bit more complicated..
               // The indicator isn't precise byte-wise... only block-wise
               // so how far can we safely skip ahead
               //  and in-doing-so can we *not* redetect what got us here
               //  in the first place...?
               if(researching == 2)
                  researching = FALSE;
               else
                  researching--;
            }
         */    
               //We're just getting indicators, at this point...
               // there may be more data, but maybe not...
               //if(((extractedData & 0x0c) == 0x0c))
               if(dataType == INDICATOR)
               {
                  indicatorCount++;

                  //No sense in reading back all the indicators
                  // if more data was received, we can skip ahead
                  // and there'll be more indicators
                  // indicating that we should jump back and look for
                  // the source...
                  // DO NOT BE FOOLED!
                  // SEEK_AMOUNT/4 amounts to 30 seconds, rather than 15
                  // why? Can't quite figure it out...
                  // because indicators occur on every *2* bytes (samples)
                  // ???
                  // and indicatorCount is only incremented *once* per
                  //  sample... so we're comparing two dissimilar values...
                  //  one's in samples, and one's in bytes.
                  if(indicatorCount > SEEK_AMOUNT/4)
                  {
                     printf("Indicator-following Timed-Out @ ");
                     printPosition2(useUsageTable, thisStartChunk, seek_o, 
                                                                  TRUE);
                     indicatorCount = 0;
                     researching = FALSE;
                  }
               }
               //We've reached the end of the noted data...
               //else if(((extractedData & 0x0c) == 0x00))
               else if(dataType == NADA)
               {
                  v_printf(1, "End of noted-data reached @ ");//
                              // 0x%"PRIx64"\n", seek_o);
                  //printPosition(seek_o, TRUE);
               
                  v_check(1)
                     printPosition2(useUsageTable, thisStartChunk, seek_o, TRUE);

                  indicatorCount = 0;
                  researching = FALSE;
               }
            }

            //So 0x00's at the beginning should be skipped-over
            // as should 0x0C's
            // an immediate 0x00 *after* data's found would cause the next
            // search
            // a bunch of 0x0C's *after* data's been found, will too...

            //If we're still researching, then only increment one sample
            if(researching)
               seek_o += 2;
            else if((seek_o + SEEK_AMOUNT > thisEndByte) && endTest)
            {
               soughtAmount_o = (thisEndByte - 1) - seek_o;
               seek_o += soughtAmount_o;
               endTest = FALSE;
               printf("SeekAmount Limited after researching to: ");
               printPosition(soughtAmount_o, TRUE);

            }
            else
            {
               soughtAmount_o = SEEK_AMOUNT;
               seek_o += SEEK_AMOUNT;
            }
         }
         else //Not Researching (searching for *anything*)
         {
            //if((extractedData & 0x0c) != 0x00)
            if(dataType != NADA)
            {
               v_printf(1, "Found data before "); //0x%"PRIx64"\n", seek_o);
               //printPosition(seek_o, TRUE);

               v_check(1)
                  printPosition2(useUsageTable, thisStartChunk, seek_o, TRUE);
               
               //Let's search for the start...
               seek_o -= soughtAmount_o; //SEEK_AMOUNT;
               v_printf(1, " looking for source, from ");// 0x%"PRIx64"\n", seek_o);
               //printPosition(seek_o, TRUE);
               v_check(1)
                  printPosition2(useUsageTable, thisStartChunk, seek_o, TRUE);

               researching = TRUE;
            }
            //else //if ((extractedData & 0x0c) == 0x00)
            else if((seek_o + SEEK_AMOUNT > thisEndByte) && endTest)
            {
               soughtAmount_o = (thisEndByte - 1) - seek_o;
               seek_o += soughtAmount_o;
               endTest = FALSE;
               v_printf(1, "SeekAmount Limited to: ");

               v_check(1)
                  printPosition(soughtAmount_o, TRUE);
            }     
            else
            {
               soughtAmount_o = SEEK_AMOUNT; 
               seek_o += SEEK_AMOUNT;
            }

         }

         //This should now handle all endByte cases...
         //if(limit_o && (seek_o > limit_o))
         if(seek_o > thisEndByte)
         {
//#warning "Should check that other loop-globals are reset as necessary"
            indicatorCount = 0;
            researching = FALSE;
            printf("Reached Search Limit @ ");
            printPosition2(useUsageTable, thisStartChunk, seek_o, TRUE);
            //exit(0);
            break;
         }
      }

      //since FragListIndex was incremented before the search
      //  this'll test whether it's reached the end *after* processing
      //  the last fragment
      } while(useUsageTable && (fragList2[fragListIndex].firstChunk != -1));
            //(sortedUTindices[thisSUTindex] != -1));


      exit(0);
   }




















int timeStringFromBytes(char* string, off_t bytes)
{
   uint64_t secTotal = (bytes / (sampleRate * FILE_SAMPLE_BYTES));
   uint64_t hours = secTotal / (60*60);
   uint64_t minutes = (secTotal - (hours*60*60))/60;
   uint64_t secs = secTotal - hours*60*60 - minutes*60;

   int printedChars;

   printedChars = sprintf(string, "%"PRIu64"h%02"PRIu64"m%02"PRIu64"s",
   //printedChars = sprintf(string, "%"PRIu64"h%"PRIu64"m%"PRIu64"s",
         hours, minutes, secs);

   return printedChars;
}

int printTimeFromBytes(off_t bytes)
{
   int printedChars = 0;
/*
   uint64_t secTotal = (bytes / (sampleRate * FILE_SAMPLE_BYTES));
   uint64_t hours = secTotal / (60*60);
   uint64_t minutes = (secTotal - (hours*60*60))/60;
   uint64_t secs = secTotal - hours*60*60 - minutes*60;

   printedChars = printf("%"PRIu64"h%"PRIu64"m%"PRIu64"s",
                           hours, minutes, secs);
*/
   char timeString[20];

   timeStringFromBytes(timeString, bytes);
   printedChars += printf("%s", timeString);

   fflush(stdout);
   return printedChars;
}


int printPosition(off_t position_o, uint8_t printReturn)
{
   int printedChars = 0;
/*
   uint64_t secTotal = (position_o / (sampleRate
                                       * FILE_SAMPLE_BYTES));
   uint64_t hours = secTotal / (60*60);
   uint64_t minutes = (secTotal - (hours*60*60))/60;
   uint64_t secs = secTotal - hours*60*60 - minutes*60;
*/ 
   printedChars =
         printf("0x%"PRIx64" = ", position_o); //%"PRIu64"s = %"
                  // PRIu64"h%"PRIu64"m%"PRIu64"s",
                  //         position_o, secTotal, hours, minutes, secs);

   printedChars += printTimeFromBytes(position_o);

   if(printReturn)
      printedChars += printf("\n");

   fflush(stdout);

   return printedChars;
}

uint64_t scanPosition(char *string)
{
   uint64_t hexVal = 0;
   uint8_t scanRet;
   scanRet = sscanf(string, "0x%"SCNx64, &hexVal);
   if(scanRet >= 1)
      return hexVal;


   uint32_t h=0, m=0, s=0;
   scanRet = sscanf(string, "%"SCNu32"h%"SCNu32"m%"SCNu32"s", &h,&m,&s);
   //Was hoping >= 1 would pick up at least 1h, and 1h2m
   // but it also picks up 1k and just 1
   // So basically, any following-characters are worthless...
   //So basically you HAVE to do 0h1m2s, NOT 1m2s, etc...
   if(scanRet == 3)
   {
//    printf("%"PRIu32"h%"PRIu32"m%"PRIu32"s = %"PRIu32"s\n", h,m,s,
      return (uint64_t)(sampleRate) * (uint64_t)(FILE_SAMPLE_BYTES) 
            * ((uint64_t)h*(uint64_t)60*(uint64_t)60 
                + (uint64_t)m*(uint64_t)60 + (uint64_t)s);
   }



   //[mkb] would be nice, but it returns a string, which mightn't
   // be logical, so testing is necessary anyhow... e.g. mk
   uint64_t val = 0;
   char character = '\0';
   scanRet = sscanf(string, "%"SCNu64"%c", &val, &character);
   if(scanRet >= 1)
   {
      switch(character)
      {
         case '\0':  //Not Assigned
         case 'b':
         case 'B':
            //nothing to do
            break;
         case 'k':
         case 'K':
            return val*(uint64_t)1024;
            break;
         case 'm':
         case 'M':
            return val*(uint64_t)1024*(uint64_t)1024;
            break;
         case 'g':
         case 'G':
            return val*(uint64_t)1024*(uint64_t)1024*(uint64_t)1024;
            break;
         default:
            return SCAN_UNHANDLED;
      }

   }

   //Unhandled...
   return SCAN_UNHANDLED;
}



   //ChunkTime Determination:
   // (assuming high-nibble is boot-num, low-nibble is loop-num)
   // (and usageTable is 8 chunks long)
   // (and chunk-Time is 5min)
   //
   //  0    1    2    3    4    5    6    7
   // 0x00 0x01 0x00 0x10 0x10 0x20 0x00 0x31
   //
   // chunk 0: (0x00)
   //   No marking, no time
   // chunk 1: (0x01)
   //   First Boot
   //   Second Chunk
   //   Nothing prior
   //   Starts at 5:00
   // chunk 2: (0x00)
   //   No Marking, No Time
   // chunk 3: (0x10)
   //   Second Boot
   //   First loop
   //   Chunk 1 was full, so traversed: 0 -> 2 -> 3
   //            0-5min, 5-10min, 10-15min
   //   Starts at 10min from boot
   // chunk 4: (0x10)
   //   Second Boot
   //   First Loop
   //   Chunk 1 was full
   //   Chunk 3 was thisLoop
   //   Starts at 15min from boot
   // chunk 5: (0x20)
   //   Third Boot
   //   First loop
   //   0 -> 2 -> 5
   //   Starts at 10min from boot
   // chunk 6:
   //   nada
   // chunk 7:  (0x31)
   //   Fourth Boot
   //   Second Loop
   //   0 -> 2 -> 6 -> 7 -> 0 -> 2 -> 6 -> 7
   //        5    10   15   20   25   30   35
   //   Starts at 35min from boot

   //And if a chunk is filled from this boot...?
   // e.g.
   //           vvvv
   //  0    1    2    3    4    5    6    7
   // 0x00 0x01 0x30 0x10 0x10 0x20 0x00 0x31
   // Chunk 2:
   //   Fourth Boot
   //   First Loop
   //   0 -> 2 = 5min
   // Chunk 7:
   //   Fourth Boot
   //   Second Loop
   //                         /-----\ No 2 between
   //   0 -> *2* -> 6 -> 7 -> 0 -> *6* -> 7
   //         5     10   15   20    25    30
   //   Starts at 30min from boot.


//RL example:
//        0  1  2  3  4  5  6  7    8  9  a  b  c  d  e  f
//0x00:  01 20 40 40 40 60 80 a0   c0 c1 20 40 40 e0 e1 e2
//1         0x20        1 (0x01->0x01)   0x0 = 0s = 0h0m0s
//2         0x20        1 (0x0a->0x0a)   0x41f4000 = 1798s = 0h29m58s
//3         0x40        3 (0x02->0x04)   0x0 = 0s = 0h0m0s
//4         0x40        2 (0x0b->0x0c)   0x3aa0000 = 1598s = 0h26m38s
//
// How is it that 2 and 4 return different values?!
// Because 0x0a is marked 0x20 before loop 0x40, and therefore skipped
//
// So then, of course (0x0a-0x01 != 0x0b-0x02)


//This will return the number of chunks *not including* the one we're
// looking for... e.g. utPos = 0, ut[0] = 1, will return 0
uint32_t chunksSinceBoot_fromUTPosition(uint8_t utPos)
{
   //uint16_t i;

   //NEW usageTable boot/loop arrangement a/o audioThingAVR27 (NYI!)
   // this should work with most if not all recordings prior
   // since loopNum > 8 is seldom...

   //actual ut Value, including bootNum...
   uint8_t chunkLoopNum = ut[utPos]; // & 0x0f;
   //uint8_t chunkBootNum = ut[utPos] >> 4;
   

   uint32_t chunkCount = 0;

   //Start at this BootNum, loop 0
   uint16_t thisLoopNum = chunkLoopNum & 0xf0;

   //If chunkLoopNum = 0x01->0x0f, thisLoopNum will start at 0x00
   // But 0x00 isn't a loop number...
   // so skip it.
   if(thisLoopNum == 0)
      thisLoopNum = 1;

   for( ; thisLoopNum <= chunkLoopNum ; thisLoopNum++)
   {
      uint16_t thisUTpos;
      //Iterate through the whole utArray for each loop in loopNum
      // (since boot)
      for(thisUTpos=0; thisUTpos<256; thisUTpos++)
      {
         uint8_t thisUTval = ut[thisUTpos];

         //Done
         if((thisLoopNum == chunkLoopNum) && (thisUTpos == utPos))
         {
            return chunkCount;
         }
      
         //Count unmarked chunks, as well as those that were marked
         // *during or after* this same loopNum
         if((thisUTval == 0) || (thisUTval >= thisLoopNum))
         {
            chunkCount++;
         }

      }
   }

   //This shouldn't happen normally...
   return 0;
}


// useUT:
//   TRUE: then it prints values from the start-time of the loop...
//   FALSE: just call printPosition()
// startingChunkNum:
//   usually the chunk-number (or position in the UT)
//   that starts the currently-processed fragment
// seek_o is the current file position, in bytes
//   Can't just set this to zero...
// printReturn if TRUE adds a newline to the end of the printout
void printPosition2(uint8_t useUT, uint8_t startingChunkNum, 
                     off_t seek_o, uint8_t printReturn)
{
   if(useUT)
   {
      uint8_t bootLoopNum = ut[startingChunkNum] & 0xf0;

      //loop 0 is non-existent!
      if(bootLoopNum == 0)
         bootLoopNum = 1;

      printf("0x%"PRIx64". From BootLoopNum=0x%02"PRIx8": ", seek_o, 
                  bootLoopNum);

      //This Ain't Right!!!
      //printPosition(seek_o - startBytesSinceBoot, TRUE);

// seek_o is somewhat irrelevent in terms of the position since boot, no?
//
// need: UT_chunkStartByte, then 
//    bytePositionInChunk = seek_o - chunkStartByteUT
//
// NOT the same:
// byteInFile = seek
// byteSinceBoot (from chunksSinceBoot)

// Do we even know the ut position aka chunkNum we're in? 

//    bytePositionSinceBoot = - bootBytePosition


      // Byte Number in the file, of startingChunkNum
      off_t start_fileByte = startingByteNumFromChunk(startingChunkNum);

      // We know that all the bytes in this loop will be consecutive
      // Regardless of where/when this chunk started in a loop, or
      // in the UT...
      // so the number of bytes gotten since the first byte in the
      // chunk is:
      off_t fileBytesGottenSinceStartOfFragment =
                     seek_o - start_fileByte;

      // This, converted to bytes is the number of bytes recorded
      //  since boot, as-of the first chunk in this fragment
      uint32_t chunksSinceBoot_firstChunkInFragment =
               chunksSinceBoot_fromUTPosition(startingChunkNum);
               
      off_t bytesSinceBoot_firstChunkInFragment =
               calcBytesFromChunks(chunksSinceBoot_firstChunkInFragment);

      // And this is the number of bytes recorded in since boot
      // up to this point.
      off_t bytesSinceBoot_tillNow =
               bytesSinceBoot_firstChunkInFragment +
               fileBytesGottenSinceStartOfFragment;

   
      //printPosition(bytesSinceBoot_tillNow, printReturn);    
      printTimeFromBytes(bytesSinceBoot_tillNow);
      if(printReturn)
         printf("\n");
   }
   else
      printPosition(seek_o, printReturn);
}


//a/o v7p13 (Switchover to Debian)
// Assuming that the data-skew errors is due to switchover between
//  endianness...
// Then, it's no longer possible to use fread to directly-read the
//  samples.
// Instead, we need to read the bytes and order-them as appropriate
size_t freadSamples(uint16_t *sampleBuffer, size_t numSamples, FILE* fid)
{
   //see cTests/sizeofArray.c:
   // sizeof(arrayName) is the size of the array itself *in bytes*
   // NOT the size of the pointer to the first element, 
   //  that the name actually refers to
   // NOR the number of elements in the array.
   // (This should be portable, right?)
   uint8_t 
      buffer_1ch16b_unorderedBytes[BUFFER_1CH16B_SIZE*BYTES_PER_SAMPLE];

   if(numSamples > BUFFER_1CH16B_SIZE)
   {
      printf("WHOOPS! Requested too many samples!\n");
      printf(" This isn't really handled...\n");
   } 


   size_t numInBytes = 
      fread(buffer_1ch16b_unorderedBytes, sizeof(uint8_t),
            numSamples*BYTES_PER_SAMPLE, fid);

   //audioThing[AVR] stores the high-byte first, then the low-byte
   size_t sampleNum = 0;
   
   for(sampleNum = 0; sampleNum < numInBytes/2; sampleNum++)
   {
      sampleBuffer[sampleNum] =  
          ((buffer_1ch16b_unorderedBytes[sampleNum*2])<<8)
         | (buffer_1ch16b_unorderedBytes[sampleNum*2+1]) ;
   }

   return sampleNum;
}




int loadHeader(void)
{
   int numInBytes =
      fread(&headerData, sizeof(uint8_t), HEADER_SIZE, fid);

   if(numInBytes != HEADER_SIZE)
   {
      printf("Unable to read the header (first %d bytes)\n", HEADER_SIZE);
      return -1;
   }

   int i = 0;

   int headerType = NO_HEADER;

   if(0==strncmp((char*)headerData, "asdf", 4))
   {
      printf("This Card has been marked with 'asdf', which is my quick \n"
             " method to cause the card to be reformatted next time \n"
             " Are you sure you want to continue with it? [y/n]");
      int goOn = getchar();

      if(goOn == 'y')
         i=6; //skip the "asdf" in testing the header-format, it may have
              // newlines, etc. from echo...
      else
         exit(1);
   }

   //a/o audioThing v60 and audioThing-desktop v7p20:
   // The FORMATHEADER now contains the sample-rate as a string in the last
   // 8 bytes... We'll attempt to extract that here, and if it's not there,
   // we'll assume we're working with an old version...
   uint8_t headerEndSkip = 0;
   
   //Because there are 8 bytes available for the sample-rate, and the
   //sample-rate is in samples/sec, it'd be *very unlikely* we'd use more
   //than 5, certainly no more than 6 non-null characters...
   //The remainder are '\0'
   // (e.g. "19230")
   //So, for now, we're defining that the sample-rate is stored in the
   //format-header if the last two characters are '\0'
   if((headerData[FORMATHEADER_SIZE-1] == '\0') &&
      (headerData[FORMATHEADER_SIZE-2] == '\0'))
   {
      printf("FormatHeader: SampleRate Bytes: '%s'\n",
            (char*)(&(headerData[FORMATHEADER_SIZE-8])));
      //The 6th byte can be a "?" to indicate that the value was assumed,
      //not so much for extracted-files, but for extraction from an
      // audioThing card that didn't have sampleRate info. 
      char character = '\0';

      int scanCount = 
         sscanf((char*)&(headerData[FORMATHEADER_SIZE-8]), "%d%c",
                                              &sampleRate, &character);

      //sampleRate has been written...
      if(scanCount >= 1)
      {
         offsetDelay = OFFSET_DELAY_MULTIPLIER * sampleRate;
         headerEndSkip = 8;

         printf("Potential Sample-Rate read from the Potential format-header: %dS/s\n",
                                                            sampleRate);

         if(scanCount == 1)
            srAssumed=FALSE;
         else if((scanCount == 2) && (character == '?'))
         {
            printf(" This value is marked as 'assumed'\n");
            srAssumed=TRUE;
         }
      }
      else
         printf("Unable to read Sample-Rate from the format-header\n"
       "Either the format is incorrect or this is an old audioThing file\n"
                "Assuming sample-rate = 19230S/s\n");

      //either headerEndSkip has been set, or it's still 0
      // in which case, the old header-processing should still work
      // (and obviously fail)
   }
   
   
   
   
   //i will either start at 0, or 6...
   for(; i<FORMATHEADER_SIZE-headerEndSkip; i++)
   {
      //Since there's likely a case where a single byte matches both
      // auT and auTx headers, make sure we don't rewrite the
      // headerType in that case...
      // mathematically, I suppose, such a case can't exist 255/2 != int
      //a/o v7p13: Not really sure this makes sense...
      if(i == 255-i)
      {
         if(headerData[i] != i)
         {
            headerType = NO_HEADER;
            break;
         }
      }
      if(headerData[i] == i)
      {
         //audioThing[AVR] header, usually read straight from the SD-Card
         headerType = auT_HEADER;
      }
      else if(headerData[i] == 255-i)
      {
         //audioThing-Extraction header
         // These are usually files extracted from the SD-Card
         // via audioThing[Desktop]
         headerType = auTx_HEADER;
      }
      else
      {
         headerType = NO_HEADER;
         break;
      }
   }

   if((i != FORMATHEADER_SIZE-headerEndSkip) || (headerType == NO_HEADER))
   {
      printf("Unrecognized Header\n");
      headerType = NO_HEADER;
   }
   else if(headerType == auTx_HEADER)
   {
      printf("This is an AudioThing-Extraction, with the following info:\n");
      printf("%s", &(headerData[256]));
   }
   else if(headerType == auT_HEADER)
   {
      printf("This is an AudioThing file, straight from the device.\n");
   }
   else
   {
      printf("WTF? Shouldn't get here...\n");
      headerType = NO_HEADER;
   }  
   
   
   return headerType;
}



int loadUT(void) //int *loopMax)
{
      int i;

      printf("Usage Table:\n");
      printf("        0  1  2  3  4  5  6  7    8  9  a  b  c  d  e  f\n");
      
      for(i=0; i<USAGETABLE_SIZE; i+=0x10)
      {
         printf("0x%02x: ", i);

         int j;
         for(j=0; j<=0x0f; j++)
         {
            if(j==8)
               printf("  ");
            printf(" %02x", ut[i+j]);
/*
            //For later...
            if(ut[i+j] > *loopMax)
               *loopMax = ut[i+j];
*/       }

         printf("\n");
      }

      return 0;
}

//This is to be called sequentially...
// linkedWithLast should be TRUE if the new chunk is temporally-sequential
void fl2_add(int chunkNum, uint8_t linkedWithLast)
{
   static int fl2_index = 0;
   static int firstGo = TRUE;

   //Check if the previous fragment is to be extended...
   // ChunkNum is one greater than the last one added and
   // LoopNum is the same
   if( !firstGo
       && (fragList2[fl2_index].loopNum == ut[chunkNum])
       && (chunkNum == fragList2[fl2_index].lastChunk + 1 ) )
   {
      fragList2[fl2_index].lastChunk = chunkNum;
   }
   //Otherwise, it's a new fragment
   else
   {
      if(!firstGo)
      {
         //Update the last fragment's linkedness
         fragList2[fl2_index].linkedWithNext = linkedWithLast;

         fl2_index++;
      }
      
      //** Create the next fragment

      fragList2[fl2_index].firstChunk = chunkNum;
      //This may be extended
      fragList2[fl2_index].lastChunk = chunkNum;
      fragList2[fl2_index].loopNum = ut[chunkNum];
      //This will be modified later, as appropriate
      fragList2[fl2_index].linkedWithNext = FALSE;
      fragList2[fl2_index].linkedWithLast = linkedWithLast;

      firstGo = FALSE;
   }
}


int sortUT(void) //int loopMaxUnused)
{
//    int sortedUTindices[USAGETABLE_SIZE+1] = 
//                               { [0 ... USAGETABLE_SIZE] = -1};

//    int sortUT_index = 0;

      int loopNum = 1;
      // Sort the usage-table into sortUT[]
      int nextMin = 0xffff;
      int loopMax = 0;

      int i;

      int linkedWithLast = FALSE;

      int markedChunkCount = 0;

      printf("Marked chunks sorted by loopNum: (ut_index)\n");
#warning "TODO: This apparently crashes if the ut is empty?"
      while(1) //will use break...
      {
         nextMin = 0xffff;
   
         for(i=0; i<USAGETABLE_SIZE; i++)
         {
            //Determine the last loop (in order to stop cycling)
            //(It's not used until after this loop completes)
            if(ut[i] > loopMax)
               loopMax = ut[i];

            //Determine which loopNum to handle next
            // (The result of which is not valid until the whole
            //  loop has completed)
            if((ut[i] > loopNum) && (ut[i] < nextMin))
               nextMin = ut[i];
   
            //Determine whether the next chunk in this loopNum
            // is temporally consecutive
            if((ut[i] == 0) || (ut[i] > loopNum))
               linkedWithLast = FALSE;



            //Add matches...
            if(ut[i] == loopNum)
            {
               fl2_add(i, linkedWithLast);

               linkedWithLast = TRUE; //For the next chunk
               // It will be falsified if necessary

               sortedUTindices[sortUT_index] = i;
               sortUT_index++;
               //sortedUTindices[sortUT_index] = -1; 
               //Indicate the end of list

               // but *can* be ==
               if(sortUT_index > USAGETABLE_SIZE)
               {
                  printf("WTF?!\n");
                  return 1;
               }

               printf("%02x", i);

               //fflush(stdout);
               markedChunkCount++;
               if(markedChunkCount % 24 == 0)
                  printf("   (%d)\n", markedChunkCount);
               else
                  printf(" ");
            }
         }

         //THIS IS WHERE IT GOT STUCK:
         // (I'm guessing)
         // ... prior to v7p21
         // ... when the usage-table is empty (every byte is 0)
         // Since loopNum == 1, and loopMax == 0, loopNum keeps increasing
         //if(loopNum == loopMax)  //We're done...
         //There shouldn't be any other case where loopNum could increment
         //beyond loopMax, right...?
         if(loopNum >= loopMax)
            break;
         else
         {
            if(nextMin != loopNum+1)
               linkedWithLast = FALSE;
            
            loopNum = nextMin;
         }


      }
      
      printf("   (%d)\n", markedChunkCount);
      printf("%d chunks marked\n", markedChunkCount);
      printf(" Total Bytes: ");
      //Coulda used calcBytesFromChunks...
      printPosition(numBlocksPerChunk*512*markedChunkCount, TRUE);

      return 0;
}


            //      0 1 2 3 4 5 6    (UTindex, chunkNum)
            // ut:  0 5 5 5 4 0 3    (chunkLoopNum)
            //        \___| ^   ^
            //            | |   |
            //       _____|_|___/
            //      /     | |
            //      |  ___|_/
            //      | /  _|_    ___ End of list
            //      v v /   \  /
            // sortedIndices:  v
            //      6 4 1 2 3 -1     (UTindex)
            //      0 1 2 3 4  5     (SUTindex)
void printFragList(void)
{
      //Print the fragment info:
      int lastLoopNum = 0xffff;

      int i;

      for(i = 0; fragList2[i].firstChunk != -1; i++)
      {
         int printBootNum = FALSE;
         int printLoopNum = FALSE;
         int printHeader = (lastLoopNum == 0xffff) ? TRUE : FALSE;


         if(fragList2[i].loopNum != lastLoopNum)
         {  
            int thisBootNum = ((fragList2[i].loopNum)>>4);

            if(thisBootNum != (lastLoopNum>>4))
            {  
               if(lastLoopNum != 0xffff)
                  printf("\n");
               printBootNum = TRUE;
               //printf(" Boot %d (Most Cases):\n", thisBootNum);
            }
            //printf(   "  Loop %d (Most Cases):\n", thisLoopNum);
            printLoopNum = TRUE;

            lastLoopNum = fragList2[i].loopNum;
         }


         printFragInfo(i, printHeader, printBootNum, 
                                             printLoopNum, FALSE);
      }
}

         

void printFragInfo(int fragNum, int printHeader, int printBootNum,
                                int printLoopNum, int printFilePositions)
{
   if(printHeader)
   {
      printf(
"Fragment    LoopNum    Chunks            Time (Since Boot)       Duration\n"
"--------    -------    ------            -----------------       --------\n");
   }
   
   if(printBootNum)
   {
      int thisBootNum = ((fragList2[fragNum].loopNum)>>4);

      printf(" Boot %d (Most Cases):\n", thisBootNum);
   }        
   
   if(printLoopNum)  
   {
      int thisLoopNum = ((fragList2[fragNum].loopNum)&0x0f);

      //Since the first boot starts with loopNum 0x01
      if(((fragList2[fragNum].loopNum)>>4) == 0)
         thisLoopNum--;

      printf(   "  Loop %d (Most Cases):\n", thisLoopNum);
   }


   //This math is safe because no fragment wraps around to another loop
   // Right...?
   int numChunksInFragment = fragList2[fragNum].lastChunk
                              - fragList2[fragNum].firstChunk + 1;



   printf("%5d       0x%02"PRIx8
                          "       %2d (0x%02"PRIx8
                                     "->0x%02"PRIx8")  ",
      fragNum, fragList2[fragNum].loopNum, 
      numChunksInFragment,
      fragList2[fragNum].firstChunk,   fragList2[fragNum].lastChunk);


   if(fragList2[fragNum].linkedWithLast)
      printf("<");
   else
      printf(" ");


   char sts[20];
   char ets[20];

   timeStringsFromFrag(fragNum, sts, ets, FALSE);

   int printCount = printf("%s -> %s", sts, ets);

   int j;

   if(fragList2[fragNum].linkedWithNext)
      printf(">");
   else
      printf(" ");

   for(j=printCount; j<23; j++)
      printf(" ");

   printTimeFromBytes(calcBytesFromChunks( numChunksInFragment ) );
   printf("\n");
/* This was just a test for linkify
   if(timeStringsFromFrag(fragNum, sts, ets, TRUE))
      printf("     (memo: %s -> %s)\n", sts, ets);
*/
   if(printFilePositions)
   {
      off_t startByte, endByte;

      getFragBytes(fragNum, &startByte, &endByte);

      printf("          File Bytes: 0x%"PRIx64" -> 0x%"PRIx64"\n",
            startByte, endByte);
   }
}

/*
      printf("Unmarked Chunks from LoopNum=%d (the last recorded)\n",
                     loopMax);
      printf("Actually, this isn't necessarily true;\n"
           "e.g. another loop started after the last *marked* loop...\n");

      printf("Probably better to run this on a per-case basis...\n"
            "e.g. when more playback history is desired than was marked\n"
            "the best we can do is *try* the previous unmarked chunk\n");

      printf("\nThese *might* belong to LoopNum=%d\n", loopMax);
      //e.g. use findPriorChunk (from _avr/audioThing)
      //       instead of this...
      // Though some info is known... certain unmarked blocks 
      //  *can't have been* last-recorded prior to the last marked-block
      //  following it...
      //           0 1 2 3 4 5 6 7 8
      // e.g.  ut: 0 5 0 6 0 2 0 0 0
      // ut[0, 2] MUST have been last recorded AT OR AFTER loopNum 6
      // ut[4]    AT OR AFTER loopNum 2
      // ut[6-8]  Completely unknown
      // AND bootNums make it way more complicated...

      //Determine what we can about inbetween chunks...
      // (which loop were they recorded?)
      // Best I know, so far, is 
      //   any unmarked chunks
      //      between those marked with the lastLoopNum
      //      musta been written by lastLoopNum
      // NOT! See printf, above.
      int lastLoopNumFound = FALSE:
      for(i=(USAGETABLE_SIZE-1); i >= 0; i--)
      {
         if(ut[i] == lastLoopNum)
            lastLoopNumFound = TRUE;

         //Unmarked chunks before the last chunk marked with lastLoopNum
         if((lastLoopNumFound) && (ut[i]==0))
            printf("%02x ", i);
      }
*/


int getFragBytes(int fragListIndex, off_t *startByte, off_t *endByte)
{
   if(fragList2[fragListIndex].firstChunk == -1)
      return -1;

   *startByte = startingByteNumFromChunk(
                                 fragList2[fragListIndex].firstChunk);
                  
   *endByte = startingByteNumFromChunk(
                                 fragList2[fragListIndex].lastChunk+1)-1;

   return 0;   
}


//SEARCH NOTES:            
//#warning "THIS IS HARDLY well-implemented"
//Considerations: chunk sizes may be larger (or smaller) than SEEK_AMOUNT
// Shouldn't search a prior chunk
// Should check the last sample in the chunk...
// Do consecutively-marked fragments?
//    ---> consecutively-marked in 0xff aren't necessarily 
//         consecutive-writes
//          thisChunkStartByte = 
//             byteNumFromChunkNum(sortedUTindices[sortUTindex]);


//Time/Byte Notes:
//This doesn't take into account the first sector...
// a few seconds, bfd, right?
// Not even... not even a fraction of a second
//printf("Starting At: ");
//printTimeFromBytes(startBytesSinceBoot);

//If linkify is TRUE, it gives the start/end times for
// the full memo containing fragNum...
//Returns the number of fragments sought prior and after
// in two bytes of int...
int timeStringsFromFrag(int fragNum, char *startTimeString, 
                        char *endTimeString, int linkify)
{
   int soughtPrior = 0;
   int soughtAfter = 0;

   if(linkify)
   {
      while(fragList2[fragNum].linkedWithLast)
      {
         soughtPrior++;
         fragNum--;
      }
   }


   if(startTimeString != NULL)
   {
      uint32_t chunksSinceBoot =
             chunksSinceBoot_fromUTPosition(fragList2[fragNum].firstChunk);
      //Start Time
      timeStringFromBytes(startTimeString, 
                                 calcBytesFromChunks(chunksSinceBoot));
   }


   if(linkify)
   {
      while(fragList2[fragNum].linkedWithNext)
      {
         soughtAfter++;
         fragNum++;
      }
   }

   if(endTimeString != NULL)
   {
      //End Time
      uint32_t endChunksSinceBoot =
         chunksSinceBoot_fromUTPosition(fragList2[fragNum].lastChunk);
      
      timeStringFromBytes(endTimeString,
                              calcBytesFromChunks(endChunksSinceBoot+1)-1);  
   }


   return (((soughtAfter&0xff)<<8) | (soughtPrior&0xff));
}



void printProgressBar(off_t done, off_t total)
{
   off_t scrollCount = done*60/total;

   int i;

   printf("\r|");

   for(i=0; i<scrollCount; i++)
      printf("#");
   for( ; i<60; i++)
      printf("-");
   printf("| (%d%%)", (int)(done*100/total));
                  
   fflush(stdout);
}

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
 * /home/meh/audioThing/7p16-git/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
