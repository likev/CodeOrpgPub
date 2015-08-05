# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 19:00:37 $
# $Id: signals.mak,v 1.2 2014/02/28 19:00:37 steves Exp $
# $Revision: 1.2 $
# $State: Exp $

# Template make description file for describing a C binary file

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

# Flags to be passed to compiler
#CCFLAGS = -g -pipe -O2 -fno-strength-reduce $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
CCFLAGS = -g $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =
DEPENDFILE =

all:: $(ARCH)/signals.o 
