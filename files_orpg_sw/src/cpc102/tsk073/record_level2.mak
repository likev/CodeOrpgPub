# RCS info
# $Author: jeffs $
# $Locker:  $
# $Date: 2014/03/11 18:15:18 $
# $Id: record_level2.mak,v 1.3 2014/03/11 18:15:18 jeffs Exp $
# $Revision: 1.3 $
# $State: Exp $

CONVERT_LDM_DIR = ../../cpc112/tsk001
CONVERT_LDM_INC = -I../../cpc112/tsk001
include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)
include make.record_level2

DEBUG_LOC_LDOPTS = -L$(MAKETOP)/lib/$(ARCH) -Wl,-Bstatic
DEBUG_SYS_LDOPTS = -Wl,-Bdynamic

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DLEVEL2_TOOL

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(CONVERT_LDM_INC) $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH = $(SYS_LIBPATH)

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -lorpg -linfr -lbz2 -lm 
# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES = 

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = 

lnux_x86_DEBUG_LD_OPTS = -ldl -lbz2 -lz -lcrypt
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)


# some system libraries, if needed.

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEPENDFLAGS =

SRCS =	cldm_main.c \
        cldm_init.c \
        cldm_manage_metadata.c \
        cldm_read_messages.c \
        cldm_write_records.c


TARGET = record_level2

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

-include $(DEPENDFILE)
