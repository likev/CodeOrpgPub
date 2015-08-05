# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:45:01 $
# $Id: hci.mak,v 1.52 2014/03/07 18:45:01 steves Exp $
# $Revision: 1.52 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES = -I..

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES =

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES) $(X_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES) \
	$(X_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH = $(SYS_LIBPATH) $(X_LIBPATH)

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -lhci -lrms_util -lrpgc -lorpg -linfr -lm

# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES = 

X_LIBS = -lXm -lXt -lX11 -lMrm
# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = $(X_LIBS)

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)


CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEPENDFLAGS =

SRCS	=	hci.c						\
		hci_register_callbacks.c			\
		hci_RPG_products_button.c			\
		hci_RPG_rms_button.c				\
		hci_console_message_callback.c			\
		hci_control_panel.c				\
		hci_control_panel_RMS_button.c			\
		hci_control_panel_RDA_button.c			\
		hci_control_panel_RPG_button.c			\
		hci_control_panel_USERS_button.c		\
		hci_control_panel_applications.c		\
		hci_control_panel_control_status.c		\
		hci_control_panel_draw.c			\
		hci_control_panel_erase.c			\
		hci_control_panel_elevation_lines.c		\
		hci_control_panel_environmental_winds.c		\
		hci_control_panel_expose.c			\
		hci_control_panel_input.c			\
		hci_control_panel_mode_select.c			\
		hci_control_panel_orda_alarms.c			\
		hci_control_panel_power.c			\
		hci_control_panel_radome.c			\
		hci_control_panel_rda_alarms.c			\
		hci_control_panel_rda_status.c			\
		hci_control_panel_resize.c			\
		hci_control_panel_rpg_rms_connection.c		\
		hci_control_panel_rpg_rda_connection.c		\
		hci_control_panel_rpg_users_connection.c	\
		hci_control_panel_rpg_status.c			\
		hci_control_panel_set_system_time.c		\
		hci_control_panel_set_volume_time.c		\
		hci_control_panel_status.c			\
		hci_control_panel_system_log_messages.c		\
		hci_control_panel_tower.c			\
		hci_control_panel_users_status.c		\
		hci_control_panel_vcp.c				\
		hci_control_panel_window_title.c		\
		hci_define_bitmaps.c				\
		hci_display_feedback_string.c			\
		hci_faa_redundant.c				\
		hci_force_adapt_load.c				\
		hci_force_dev_configure.c			\
		hci_free_txt_msg.c				\
		hci_timer_proc.c				\
		hci_update_antenna_position.c			\
		hci_get_rms_status.c				\
		hci_rms_text_message_callback.c			\
		hci_rms_inhibit_message_callback.c		
	

TARGET = hci

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

-include $(DEPENDFILE)
