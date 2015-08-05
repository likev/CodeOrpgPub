# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:26 $
# $Id: en_lib.mak,v 1.21 2014/02/28 18:15:26 steves Exp $
# $Revision: 1.21 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

#  Use the OLD_EN environment variable to build the non-rssd dependent
#  event notification functionality

ifdef OLD_EN
LIB_CSRCS =	en_lib.c \
		en_libcntl.c \
		en_libevts.c \
		en_libntfy.c \
		en_libpost.c \
		en_libsock.c
else
LIB_CSRCS = en.c en_client.c en_server.c
endif

LIB_TARGET = en

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

# We don't need to install this library anymore

libinstall::
	rm -rf $(LIBDIR)/lib$(LIB_TARGET).*

-include $(DEPENDFILE)
