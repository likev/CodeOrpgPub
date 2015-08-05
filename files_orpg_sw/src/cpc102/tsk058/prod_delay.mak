# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:32:34 $
# $Id: prod_delay.mak,v 1.5 2014/03/07 19:32:34 steves Exp $
# $Revision: 1.5 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# Append X_INCLUDES to the list of includes if you want to 
# use include files for X and motif.
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES)  $(X_INCLUDES)

# This is a list of all defines.
# Append X_DEFINES to the list, if appropriate
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(X_DEFINES)

# If extra library paths are needed for this specific module.
# Specify X_LIBPATH, if that is appropriate ...
LIBPATH = $(X_LIBPATH)

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -lorpg -linfr
# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


X_LIBS =  -lXm -lXt -lX11 -lMrm
EXTRA_LIBRARIES =

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS =

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)



CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

SRCS =	prod_delay.c

TARGET = prod_delay

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)
