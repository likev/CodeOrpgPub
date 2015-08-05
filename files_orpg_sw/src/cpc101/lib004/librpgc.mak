# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/02/28 18:17:13 $
# $Id: librpgc.mak,v 1.27 2014/02/28 18:17:13 steves Exp $
# $Revision: 1.27 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)
include librpgc.make_files

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

LIB_CSRCS =	rpgc_abort_processing_c.c \
		rpgc_adpt_data_elems_c.c \
		rpgc_adaptation_c.c \
		rpgc_data_access_c.c \
		rpgc_event_services_c.c \
		rpgc_inbuf_c.c \
		rpgc_init_c.c \
		rpgc_itc_c.c \
		rpgc_legacy_prod_c.c \
		rpgc_outbuf_c.c \
		rpgc_prod_compress_c.c \
		rpgc_prod_support_c.c \
		rpgc_prod_request_c.c \
		rpgc_timer_services_c.c \
		rpgc_volume_radial_c.c \
		rpgc_wait_act_c.c \
		rpgc_scan_sum_c.c \
		rpgc_site_info_callback_fx.c \
                rpgcs_time_funcs.c \
                rpgcs_vcp_info.c \
                rpgcs_data_conversion.c \
                rpgcs_latlon.c \
                rpgcs_lambert.c \
                rpgcs_miscellaneous.c \
                rpgcs_model_data.c \
                rpgcs_misc_prod_funcs.c \
                rpgcs_rda_status.c \
		rpgp_gen_prod_debug.c \
                rpgp_prod_support.c \
                rpgc_prod_functions.c \
                rpgc_blockage_data_funcs.c \
                rpgc_database_query_funcs.c \
                $(LIBRPGCS_TYPE_SRCS) 

LIB_TARGET = rpgc

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)
DEPENDFLAGS = -f $(DEPENDFILE)

include $(MAKEINC)/make.cflib

-include ./makedepend.$(ARCH)
