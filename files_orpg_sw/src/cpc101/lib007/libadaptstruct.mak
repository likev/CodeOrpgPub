# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:17:12 $
# $Id: libadaptstruct.mak,v 1.9 2014/02/28 18:17:12 steves Exp $
# $Revision: 1.9 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES = 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DC_NO_UNDERSCORE -DRPGC_LIBRARY
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

LIB_CSRCS =     vil_echo_tops_callback_fx.c     \
                mode_select_callback_fx.c     \
                hydromet_prep_callback_fx.c     \
                hydromet_rate_callback_fx.c     \
                hydromet_acc_callback_fx.c      \
                hydromet_adj_callback_fx.c      \
                layer_reflectivity_callback_fx.c        \
                storm_cell_seg_callback_fx.c    \
                storm_cell_component_callback_fx.c      \
                storm_cell_track_callback_fx.c  \
                hail_callback_fx.c      \
                cell_prod_callback_fx.c \
                vad_callback_fx.c       \
                tda_callback_fx.c       \
                radazvd_callback_fx.c   \
                mpda_callback_fx.c      \
                recclalg_callback_fx.c  \
                saa_callback_fx.c       \
                mda_callback_fx.c       \
                superob_callback_fx.c	\
                dp_precip_callback_fx.c \
		vad_rcm_heights_callback_fx.c 

LIB_TARGET = adaptstruct

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)
DEPENDFLAGS = -f $(DEPENDFILE)

include $(MAKEINC)/make.cflib

liball::
	rm -rf $(ARCH)/*.shr
libinstall::
	rm -rf $(ARCH)/*.shr

-include ./makedepend.$(ARCH)
