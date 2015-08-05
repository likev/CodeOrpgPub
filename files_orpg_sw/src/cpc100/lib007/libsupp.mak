# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:15:30 $
# $Id: libsupp.mak,v 1.28 2014/02/28 18:15:30 steves Exp $
# $Revision: 1.28 $
# $State: Exp $

# Make description file for the Support Services Library (libsupp)

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

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEPENDFLAGS =

LIB_CSRCS =	cs_cfgsup.c \
		cs_parse.c \
		le_logerr.c \
		le_save_msg.c \
		le_utils.c \
		sdq_api.c \
		css_common.c \
		css_client.c \
		css_server.c \
		deau_utility.c \
		deau_range_check.c \
		deau_files.c \
		deau_remote.c \
		orpg_xdr.c

LIB_TARGET = supp

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

# We don't need to install this library anymore

libinstall::
	rm -rf $(LIBDIR)/lib$(LIB_TARGET).*

-include $(DEPENDFILE)
