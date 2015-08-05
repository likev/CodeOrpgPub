# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/22 15:53:51 $
# $Id: manage_adapt.mak,v 1.1 2005/06/22 15:53:51 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $


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
LOCAL_LIBRARIES = -linfr
PURE_LOCALLIBS = $(LOCAL_LIBRARIES)
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
EXTRA_LIBRARIES = 

# architecture specific system libraries.
slrs_spk_LIBS = -lsocket -lelf -lnsl
slrs_x86_LIBS = -lsocket -lelf -lnsl
hpux_rsk_LIBS = -lV3
# some system libraries, if needed.
SYS_LIBS = 
X_LIBS = 

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
PURECCFLAGS = $(COMMON_PURECCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =

SRCS = ma_main.c \
	ma_read_files.c \
	ma_read_cm_sql.c \
	ma_write_files.c

ADDITIONAL_OBJS =

TARGET = manage_adapt

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)



