# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:59:20 $
# $Id: hca.mak,v 1.8 2014/03/07 18:59:20 steves Exp $
# $Revision: 1.8 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES =

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

# Add local define -DHCA_ZDR_ERROR_ESTIMATE to activate the HCA's analysis of
# ZDR calibration using the 0.2 dB dry snow assumption.
# Also add local define -DHCA_ZDR_ERROR_DYNAMIC_ADJUST to activate the
# dynamic adjustment of ZDR data based on the ZDR error estimate.

LOCAL_DEFINES = -DHCA_ZDR_ERROR_ESTIMATE

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
LOCAL_LIBRARIES = $(C_ALGORITHM_LIBS)

# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES) -lz
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES =

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = 

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)


# Flags to be passed to compiler
CCFLAGS = -g $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =

SRCS    =       hca_main.c \
                hca_buffer_control.c \
                hca_allowedHydroClass.c \
                hca_beamMLIntersection.c \
                hca_defineMembershipAndWeights.c \
                hca_degreeMembership.c \
                hca_memLookup.c \
                hca_setMembershipPoints.c \
                hca_process_radial.c \
                hca_callback_fx.c \
                median_filter_quickselect.c \
                hca_readBlockage.c


TARGET = hca

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

-include $(DEPENDFILE)
