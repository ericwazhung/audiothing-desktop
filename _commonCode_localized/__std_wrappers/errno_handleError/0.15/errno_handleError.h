//errno_handleError.h 0.15-2
//	Checks errno, if non-zero prints the error and its string
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
#define ERRHANDLE_DONT_RETURN	0


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

