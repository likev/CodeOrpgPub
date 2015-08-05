# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:04:43 $
# $Id: lb_locktst.mak,v 1.4 2014/03/07 19:04:43 steves Exp $
# $Revision: 1.4 $
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
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -linfr
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
EXTRA_LIBRARIES = 

# architecture specific system libraries.
# some system libraries, if needed.
SYS_LIBS = 
X_LIBS = 

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

SRCS =	lb_locktst.c

TARGET = lb_locktst

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)
