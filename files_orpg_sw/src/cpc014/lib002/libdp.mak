# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:07:39 $
# $Id: libdp.mak,v 1.6 2014/02/28 18:07:39 steves Exp $
# $Revision: 1.6 $
# $State: Exp $

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
ALL_INCLUDES = $(LOCAL_INCLUDES) $(STD_INCLUDES) $(LOCALTOP_INCLUDES) \
		 $(TOP_INCLUDES) 

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(XOPEN_DEFINES) $(LOCAL_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

SHRDLIBLD_SEARCHLIBS =

# For debugging and regression testing
#                dp_debug.c \
#                hca_beamMLIntersection.c \
#                dp_regression.c \

LIB_CSRCS =	dp_grid_operators.c \
                dp_s2s_accum_data_store.c \
                dp_check.c \
                dp_packet.c \
                dp_regression.c \
                dp_elev_mode9.c \
                dp_time_conversions.c

LIB_TARGET = dp

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)
DEPENDFLAGS = -f $(DEPENDFILE)

include $(MAKEINC)/make.cflib

-include ./makedepend.$(ARCH)
