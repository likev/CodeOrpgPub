# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/23 16:23:58 $
# $Id: cpc020_lib003.make,v 1.5 2005/06/23 16:23:58 ccalvert Exp $
# $Revision: 1.5 $
# $State: Exp $

# Template make description file for describing a C library

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
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(XOPEN_DEFINES) $(LOCAL_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
PURECCFLAGS = $(COMMON_PURECCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


LIB_CSRCS = 	rms_header.c \
		rms_send_ack.c \
		rms_rec_ack.c \
		rms_send.c \
		rms_pad.c \
		rms_check_resend.c \
		rms_send_status.c \
		rms_up.c \
		rms_rpg_state.c \
		rms_alarm_msg.c \
		rms_loopback_msg.c \
		rms_force_adaptation_msg.c \
		rms_rda_state.c \
		rms_rda_mode.c \
		rms_arch.c \
		rms_edit_clutter.c \
		rms_edit_bypass_map.c \
		rms_wb.c \
		rms_download_clutter.c \
		rms_edit_auth_user.c \
		rms_handle_msg.c \
		rms_edit_load_shed.c \
		rms_download_load_shed.c \
		rms_narrowband_interface.c \
		rms_download_auth_user.c \
		rms_edit_pup_id.c \
		rms_download_pup_id.c \
		rms_init.c \
		rms_rec_free_text.c \
		rms_inhibit.c \
		rms_send_free_text.c \
		rms_rda_channel.c \
		rms_rpg_control.c \
		rms_edit_nb_cfg.c \
		rms_download_nb_cfg.c \
		rms_edit_dial_cfg.c \
		rms_download_dial_cfg.c \
		rms_download_bypass_map.c \
		rms_send_record_log.c
		
LIB_TARGET = rms_message

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)
DEPENDFLAGS =  -f $(DEPENDFILE)

include $(MAKEINC)/make.cflib

-include $(DEPENDFILE)
