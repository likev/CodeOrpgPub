# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:54:58 $
# $Id: qperate.mak,v 1.4 2014/03/07 18:54:58 steves Exp $
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
LOCAL_LIBRARIES = $(C_ALGORITHM_LIBS) -ldp

# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES =

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = 

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)


# Flags to be passed to compiler
CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =

SRCS	=	qperate_main.c \
                qperate_read_AdaptSupl.c \
                qperate_read_Moments.c \
                qperate_comp_Ztime.c \
		qperate_update_rainTime.c \
                qperate_list_coord.c \
                qperate_excluded_Zone.c \
                qperate_read_Blockage.c \
                qperate_build_RainfallRate.c \
		qperate_precip_accum_init.c \
                qperate_get_BinArea.c \
                qperate_comp_IRRate.c \
                qperate_comp_RateZ.c \
                qperate_comp_RateKdp.c \
		qperate_comp_RateZ_Zdr.c \
                buildDPR.c \
                buildDPR_Packet28.c \
                buildDPR_SymBlk.c \
	        qperate_comp_PBB_Precip.c         

TARGET = qperate

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

-include $(DEPENDFILE)
