# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2008/01/04 21:42:34 $
# $Id: save_adapt.mak,v 1.5 2008/01/04 21:42:34 aamirn Exp $
# $Revision: 1.5 $
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
LOCAL_LIBRARIES = -lorpg -linfr
# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = -lorpg -linfr -lbzip2
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)

PURE_LOCALLIBS = $(DEBUG_LOCALLIBS)
QUAN_LOCALLIBS = $(DEBUG_LOCALLIBS)
PRCOV_LOCALLIBS = $(DEBUG_LOCALLIBS)

EXTRA_LIBRARIES = 

# Architecture and debug or profiling tool dependent linker options for
# slrs_spk
slrs_spk_LD_OPTS = 

slrs_spk_DEBUG_LD_OPTS = -lnsl -lsocket -lelf -lrt -lm
slrs_spk_GPROF_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)

slrs_spk_PURE_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)
slrs_spk_QUAN_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)
slrs_spk_PRCOV_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = -lm

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)

lnux_x86_PURE_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)
lnux_x86_QUAN_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)
lnux_x86_PRCOV_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)

# some system libraries, if needed.

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

PURECCFLAGS = $(COMMON_PURECCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
QUANCCFLAGS = $(COMMON_QUANCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
PRCOVCCFLAGS = $(COMMON_PRCOVCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEPENDFLAGS =

SRCS =	save_adapt.c

TARGET = save_adapt

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

-include $(DEPENDFILE)
