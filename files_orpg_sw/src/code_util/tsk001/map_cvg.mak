# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:38:15 $
# $Id: map_cvg.mak,v 1.6 2014/03/07 19:38:15 steves Exp $
# $Revision: 1.6 $
# $State: Exp $

### Makefile for Archive II Disk File Utility  AR2DISK


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 
# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# Append $(X_INCLUDES) when appropriate ...
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
# Append $(X_DEFINES) when appropriate ...
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

# If extra library paths are needed for this specific module.
# Specify $(X_LIBPATH) when appropriate ...

LIBPATH = 

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -lorpg -linfr -lrpgc
DEBUG_LOCALLIBS = -lorpg -linfr -lrpgc

EXTRA_LIBRARIES = -L/usr/local/bin

# some system libraries, if needed.
# RETAIN THE FOLLOWING FOR BACKWARD COMPATIBILITY WITH PREVIOUS BUILDS
SYS_LIBS = -lm

# changed for LINUX
ifeq ($(ARCH), lnux_x86)
X_LIBS = -lXm -lXt -lX11 -lMrm -lUil
endif

# lnux_x86
lnux_x86_LD_OPTS =


CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =

SRCS = map_cvg.c

TARGET = map_cvg

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)
