#/* mehPL:
# *    This is Open Source, but NOT GPL. I call it mehPL.
# *    I'm not too fond of long licenses at the top of the file.
# *    Please see the bottom.
# *    Enjoy!
# */
#
#
#
#
#
#
#Stolen from polled_uar_Test0.40 from hfmTest3
# TARGET gets put in the _BUILD directory... not sure how I feel about that
TARGET = audioThing-desktop

MY_SRC = main.c 



COMREL = ../..
COMDIR = $(COMREL)/_commonCode

################# SHOULD NOT CHANGE THIS BLOCK... FROM HERE ############## 
#                                                                        #
# This stuff has to be done early-on (e.g. before other makefiles are    #
#   included..                                                           #
#                                                                        #
#                                                                        #
# If this is defined, we can use 'make copyCommon'                       #
#   to copy all used commonCode to this subdirectory                     #
#   We can also use 'make LOCAL=TRUE ...' to build from that code,       #
#     rather than that in _commonCode                                    #
LOCAL_COM_DIR = _commonCode_localized
#                                                                        #
#                                                                        #
# If use_LocalCommonCode.mk exists and contains "LOCAL=1"                #
# then code will be compiled from the LOCAL_COM_DIR                      #
# This could be slightly more sophisticated, but I want it to be         #
#  recognizeable in the main directory...                                #
# ONLY ONE of these two files (or neither) will exist, unless fiddled with 
SHARED_MK = __use_Shared_CommonCode.mk
LOCAL_MK = __use_Local_CommonCode.mk
#                                                                        #
-include $(SHARED_MK)
-include $(LOCAL_MK)
#                                                                        #
#                                                                        #
#                                                                        #
#Location of the _common directory, relative to here...                  #
# this should NOT be an absolute path...                                 #
# COMREL is used for compiling common-code into _BUILD...                #
# These are overriden if we're using the local copy                      #
# OVERRIDE the main one...                                               #
ifeq ($(LOCAL), 1)
COMREL = ./
COMDIR = $(LOCAL_COM_DIR)
endif
#                                                                        #
################# TO HERE ################################################




# These are global variables in--and values specific to--main.c

# Necessary CFLAGS for portAudio
CFLAGS+=-I/opt/local/include/
CFLAGS+=-L/opt/local/lib/
CFLAGS+=-lportaudio



# Override cirBuff's uint8_t data default
#CFLAGS+=-D'cirBuff_data_t=int16_t'
#CFLAGS+=-D'cirBuff_dataRet_t=int32_t'
#CFLAGS+=-D'CIRBUFF_RETURN_NODATA=(INT16_MAX+1)'
# Override cirBuff's uint8_t length/position
#CFLAGS+=-D'cirBuff_position_t=uint16_t'


#VER_CIRBUFF=1.00
#CIRBUFF_LIB=$(COMDIR)/cirBuff/$(VER_CIRBUFF)/cirBuff
#include $(CIRBUFF_LIB).mk

VER_STDIN_NONBLOCK=0.15
STDIN_NONBLOCK_LIB=$(COMDIR)/__std_wrappers/stdin_nonBlock/$(VER_STDIN_NONBLOCK)/stdin_nonBlock
CFLAGS += -D'_STDIN_NONBLOCK_HEADER_="$(STDIN_NONBLOCK_LIB).h"'
COM_HEADERS += $(STDIN_NONBLOCK_LIB).*

VER_ERRNO_HANDLE_ERROR=0.15
ERRNO_HANDLE_ERROR_LIB=$(COMDIR)/__std_wrappers/errno_handleError/$(VER_ERRNO_HANDLE_ERROR)/errno_handleError
CFLAGS += -D'_ERRNO_HANDLE_ERROR_HEADER_="$(ERRNO_HANDLE_ERROR_LIB).h"'
COM_HEADERS += $(ERRNO_HANDLE_ERROR_LIB).*


include $(COMDIR)/_make/reallyCommon2.mk


#/* mehPL:
# *    I would love to believe in a world where licensing shouldn't be
# *    necessary; where people would respect others' work and wishes, 
# *    and give credit where it's due. 
# *    A world where those who find people's work useful would at least 
# *    send positive vibes--if not an email.
# *    A world where we wouldn't have to think about the potential
# *    legal-loopholes that others may take advantage of.
# *
# *    Until that world exists:
# *
# *    This software and associated hardware design is free to use,
# *    modify, and even redistribute, etc. with only a few exceptions
# *    I've thought-up as-yet (this list may be appended-to, hopefully it
# *    doesn't have to be):
# * 
# *    1) Please do not change/remove this licensing info.
# *    2) Please do not change/remove others' credit/licensing/copyright 
# *         info, where noted. 
# *    3) If you find yourself profiting from my work, please send me a
# *         beer, a trinket, or cash is always handy as well.
# *         (Please be considerate. E.G. if you've reposted my work on a
# *          revenue-making (ad-based) website, please think of the
# *          years and years of hard work that went into this!)
# *    4) If you *intend* to profit from my work, you must get my
# *         permission, first. 
# *    5) No permission is given for my work to be used in Military, NSA,
# *         or other creepy-ass purposes. No exceptions. And if there's 
# *         any question in your mind as to whether your project qualifies
# *         under this category, you must get my explicit permission.
# *
# *    The open-sourced project this originated from is ~98% the work of
# *    the original author, except where otherwise noted.
# *    That includes the "commonCode" and makefiles.
# *    Thanks, of course, should be given to those who worked on the tools
# *    I've used: avr-dude, avr-gcc, gnu-make, vim, usb-tiny, and 
# *    I'm certain many others. 
# *    And, as well, to the countless coders who've taken time to post
# *    solutions to issues I couldn't solve, all over the internets.
# *
# *
# *    I'd love to hear of how this is being used, suggestions for
# *    improvements, etc!
# *         
# *    The creator of the original code and original hardware can be
# *    contacted at:
# *
# *        EricWazHung At Gmail Dotcom
# *
# *    This code's origin (and latest versions) can be found at:
# *
# *        https://code.google.com/u/ericwazhung/
# *
# *    The site associated with the original open-sourced project is at:
# *
# *        https://sites.google.com/site/geekattempts/
# *
# *    If any of that ever changes, I will be sure to note it here, 
# *    and add a link at the pages above.
# *
# * This license added to the original file located at:
# * /home/meh/audioThing/7p16-git/makefile
# *
# *    (Wow, that's a lot longer than I'd hoped).
# *
# *    Enjoy!
# */
