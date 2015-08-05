# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:38:14 $
# $Id: cvg_read_db.mak,v 1.6 2014/03/07 19:38:14 steves Exp $
# $Revision: 1.6 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

ifdef STANDALONE_CVG
LOCAL_DEFINES = -DLIB_LINK_STATIC
else
LOCAL_DEFINES = -DLIB_LINK_DYNAMIC
endif

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg

LOCAL_LIBRARIES = -linfr 
                                            
# note: libbzip2.a is not always in $(HOME)/lib/$(ARCH), WHY??
#       see cpc100/lib018
# on Linux, libbz2.a / libbz2.so exists in /usr/lib, there are
#       two symbolic links to /usr/lib/libbz2.so in the local
#       directory ~/lib/lnux_x86/ with different names

# for standalone, 

ifdef STANDALONE_CVG

LOCAL_LIBRARIES = $(HOME)/lib/$(ARCH)/libinfr.a 

endif


# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES =

X_LIBS = 

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS =

lnux_x86_DEBUG_LD_OPTS = -lm
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)



CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEPENDFLAGS =

SRCS =	cvg_read_db.c \
	byteswap.c \
	assoc_array_s.c \
	product_names.c \
	helpers.c

TARGET = cvg_read_db

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)
