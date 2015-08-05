# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2014/07/21 20:05:30 $
# $Id: libhci.mak,v 1.47 2014/07/21 20:05:30 ccalvert Exp $
# $Revision: 1.47 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES = 

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
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =
ifeq ($(ARCH), lnux_x86)
SHRDLIBLD_SEARCHLIBS= $(X_LIBPATH) -lrpgc -lorpg -linfr -lXm -lXt -lX11 -lMrm -lm -lcrypt
else
SHRDLIBLD_SEARCHLIBS= $(X_LIBPATH) -lrpgc -lorpg -linfr -lXm -lXt -lX11 -lMrm -lm
endif

LIB_CSRCS	=	hci_activate_child.c		\
		hci_basedata_functions.c		\
		hci_clutter_bypass_map_functions.c	\
		hci_colors.c				\
		hci_configuration.c			\
		hci_decode_product.c			\
		hci_display_color_bar.c			\
		hci_display_radial_product.c		\
		hci_display_product_radial_data.c	\
		hci_environmental_winds_functions.c	\
		hci_find_azimuth.c			\
		hci_fonts.c				\
		hci_force_resize_callback.c		\
		hci_lock.c				\
		hci_misc_funcs.c			\
		hci_nonoperational_alg.c		\
		hci_popup.c				\
		hci_precip_status_functions.c		\
		hci_prf_product_functions.c		\
		hci_product_colors.c			\
		hci_rda_adaptation_data_functions.c	\
		hci_rda_control_functions.c		\
		hci_rda_performance_data_functions.c	\
		hci_rpg_adaptation_data_functions.c	\
		hci_rpg_install_info_functions.c	\
		hci_rpg_options.c			\
		hci_scan_info_functions.c		\
		hci_up_nb_functions.c			\
		hci_user_profile_functions.c		\
		hci_vcp_data_functions.c		\
		hci_sails_functions.c			\
		hci_validate_input_functions.c		\
		hci_window_query.c			\
		hci_wx_status_functions.c		\
		hci_gain_focus_callback.c		\
		hci_options.c				\
		hci_pm.c				\
		hci_uipm.c				\
		hci_orda_pmd_functions.c

LIB_TARGET = hci

DEPENDFILE = ./depend.lib$(LIB_TARGET).$(ARCH)

include $(MAKEINC)/make.cflib

-include $(DEPENDFILE)

