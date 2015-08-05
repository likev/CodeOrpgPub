# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:35:04 $
# $Id: gen_vcp_template.mak,v 1.5 2014/03/07 19:35:04 steves Exp $
# $Revision: 1.5 $
# $State: Exp $

# Make description file for building the RPG Alerting
# Maintenance Task

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

DEBUG_LOC_LDOPTS = -L$(MAKETOP)/lib/$(ARCH) -Wl,-Bstatic
DEBUG_SYS_LDOPTS = -Wl,-Bdynamic

LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(XOPEN_DEFINES) $(LOCAL_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
#
LOCAL_LIBRARIES = -lorpg -linfr
# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES = 

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = -lm

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)


# some system libraries, if needed.

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


SRCS = gen_vcp_template.c pulse_counts.c write_vcp_template.c

ADDITIONAL_OBJS =

TARGET = gen_vcp_template

DEPENDFILE = ./depend.$(TARGET).$(ARCH)
DEPENDFLAGS =  -f $(DEPENDFILE)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)
