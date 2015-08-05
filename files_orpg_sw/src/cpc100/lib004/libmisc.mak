# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:29 $
# $Id: libmisc.mak,v 1.13 2014/02/28 18:15:29 steves Exp $
# $Revision: 1.13 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DCMP_BZIP2_YES
# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

LIB_CSRCS =	misc_bswap.c \
		misc_rsrc.c \
		misc_sig.c \
		misc_string.c \
		misc_proc.c \
		misc_rsis.c \
		misc_table.c \
		misc_msleep.c \
		misc_unix_time.c \
		str.c \
		misc_test_options.c

LIB_TARGET = misc

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

# We don't need to install this library anymore

libinstall::
	rm -rf $(LIBDIR)/lib$(LIB_TARGET).*

-include $(DEPENDFILE)
