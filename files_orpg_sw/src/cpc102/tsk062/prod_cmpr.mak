# RCS mon
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:26:44 $
# $Id: prod_cmpr.mak,v 1.12 2014/03/07 19:26:44 steves Exp $
# $Revision: 1.12 $
# $State: Exp $

# Makefile for product compare tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

#  Do not use misalign option from master makefiles
COMMON_CCFLAGS = -O

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DNO_MAIN_FUNCT

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES) $(X_INCLUDES)
# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES) -DGCC 

# If extra library paths are needed for this specific module.
LIBPATH = $(SYS_LIBPATH) 

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -lrpgc -lorpg -linfr -lz -lbz2
# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES) -Wl,-Bdynamic -ldl -lcrypt -lz -lrt
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES = 
# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS =

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)



CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = -fpermissive $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = -fpermissive $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEPENDFLAGS =

CPPSRCS = prod_cmpr.cpp	prod_cmpr_decompress.cpp orpgpat.cpp

TARGET = prod_cmpr

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cctool

-include $(DEPENDFILE)
