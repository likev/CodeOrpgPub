# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2012/08/21 13:42:19 $
# $Id: libRPG_LDM.mak,v 1.3 2012/08/21 13:42:19 ccalvert Exp $
# $Revision: 1.3 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES =  -I$(NL2_INCLUDE) -I$(ROC_LEVEL2_INCLUDE) -I$(ROC_LDM_INCLUDE)

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES) $(X_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES) $(X_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
PURECCFLAGS = $(COMMON_PURECCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =
SHRDLIBLD_SEARCHLIBS= $(X_LIBPATH) -lorpg -linfr -lm -lcrypt -lbz2

LIB_CSRCS	=	libRPG_LDM.c

LIB_TARGET = RPG_LDM

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

-include $(DEPENDFILE)

