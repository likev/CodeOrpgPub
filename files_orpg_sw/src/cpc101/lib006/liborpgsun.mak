# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:17:13 $
# $Id: liborpgsun.mak,v 1.2 2014/02/28 18:17:13 steves Exp $
# $Revision: 1.2 $
# $State: Exp $

# The make description file for the New RPG Library (liborpg)

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# Append X_INCLUDES to the list of includes if you want to 
# use include files for X and motif.
X_INCLUDED = /usr/X11R6/include
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(X_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(XOPEN_DEFINES) $(LOCAL_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

LOCAL_LIBRARIES = -linfr -lcrypt
SHRDLIBLD_SEARCHLIBS = -lcrypt -lm

LIB_CSRCS =	orpgsun.c \
		novas.c \
		novascon.c \
		readeph0.c \
		solsys3.c 

		
LIB_TARGET = orpgsun

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)
DEPENDFLAGS = -f $(DEPENDFILE)

include $(MAKEINC)/make.cflib

-include ./makedepend.$(ARCH)
