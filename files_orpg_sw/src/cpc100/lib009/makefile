# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:25 $
# $Id: cpc100_lib009.make,v 1.8 2014/02/28 18:15:25 steves Exp $
# $Revision: 1.8 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES = -I$(MAKETOP)/src/cpc901/tsk001

LOCAL_DEFINES = -DSVR4 -DALLAPI

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =


LIB_CSRCS =	MPSapi.c \
		MPStcp.c \
		MPSudp.c \
		MPSlist.c \
		MPSutil.c \
		MPSlocal.c

LIB_TARGET = MPS

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

-include $(DEPENDFILE)
