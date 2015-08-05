# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:26 $
# $Id: cpc100_lib016.make,v 1.8 2014/02/28 18:15:26 steves Exp $
# $Revision: 1.8 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_DEFINES = -DSVR4 -DALLAPI

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =


LIB_CSRCS =	net_misc.c

LIB_TARGET = net

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

# We don't need to install this library anymore

libinstall::
	rm -rf $(LIBDIR)/lib$(LIB_TARGET).*

-include $(DEPENDFILE)
