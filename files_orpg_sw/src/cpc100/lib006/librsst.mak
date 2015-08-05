# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:29 $
# $Id: librsst.mak,v 1.11 2014/02/28 18:15:29 steves Exp $
# $Revision: 1.11 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DTHREADED
# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

LIB_CSRCS =	rss_bufd.c \
		rss_file.c \
		rss_filed.c \
		rss_lb.c \
		rss_lbd.c \
		rss_shared.c \
		rss_sharedd.c \
		rss_uu.c \
		rss_uud.c  \
		rss_rpc.c

LIBT_TARGET = rsst

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

libinstall::
	rm -rf $(LIBDIR)/lib$(LIBT_TARGET).*

-include $(DEPENDFILE)
