
include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# Some extra variables for CVG
### location of installed executables for a global installation
GLOBALINSTALLDIR = /usr/local/bin


##############################################################
##### CHANGE FOR ORPG BUILD 
## NOTE  This variable is changed for a new
## version of cvg and IT MUST BE THE SAME as $CVG_PREF_DIR_NAME
## defined in global2.h
CVG_VER_NUM=cvg9.2

#CVG_VER_DIR=$(TOOLSDIR)/$(CVG_VER_NUM)
CVG_LOCAL_CONFIG = $(MAKETOP)/tools
CVG_VER_DIR=$(CVG_LOCAL_CONFIG)/$(CVG_VER_NUM)

DEFAULT_CONF_DIR=$(CVG_VER_DIR)/.$(CVG_VER_NUM)

#end of extra variables for CVG


# CVG 9.0
##LOCAL_INCLUDES = -I../lib006 -I../lib007
LOCAL_INCLUDES =

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

ifdef STANDALONE_CVG
LOCAL_DEFINES = -DLIB_LINK_STATIC
else
LOCAL_DEFINES = -DLIB_LINK_DYNAMIC
endif

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg

## FOR CODE: the libbzip2 would have to be added for all Solaris platforms.

## FOR CVG 9.0: 1. No longer link CVG versions of libgd and libungif
##                  libgd and libgif provided with Linux are used.
##              2. No longer link static bzip library for stand-alone.

## CVG 9.0 - use libgd and libgif in the main system library ( /usr/lib in Red Hat )
##LOCAL_LIBRARIES = -lbz2 -lpng -lz -linfr $(MAKETOP)/lib/$(ARCH)/libgd.so \
##                                            $(MAKETOP)/lib/$(ARCH)/libungif.so

LOCAL_LIBRARIES = -lbz2 -lgd -lpng -lz -linfr -lm


## Need static version of libinfr for stand-alone
#################################################
ifdef STANDALONE_CVG

LOCAL_LIBRARIES = -lbz2 -lgd -lpng -lz -lm $(MAKETOP)/lib/$(ARCH)/libinfr.a

endif
#################################################


# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES =

X_LIBS = -lXm -lXt -lX11 -lMrm

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS =

lnux_x86_DEBUG_LD_OPTS = -lm
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)



CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
# THE FOLLOWING IS FOR TEST PURPOSES ONLY
#CCFLAGS = -O -Wall -std=iso9899:1990 $(ALL_INCLUDES) $(ALL_DEFINES)


DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEPENDFLAGS =



##THE FOLLOWING ARE FOR BUILD 10 CHANGES
##  CVG 8.3 new file: packet_28radial.c
##THE FOLLOWING ARE FOR BUILD 12 CHANGES
##  CVG 8.7 new file: prod_disp_legend.c
SRCS =	cvg.c \
	byteswap.c \
	grow_array.c \
	assoc_array_s.c \
	assoc_array_i.c \
	layer_info.c \
	gui_main.c \
	gui_display.c \
	prefs.c \
	prefs_load.c \
	prefs_edit.c \
	colors.c \
	history.c \
	res.c \
	prod_load.c \
	callbacks.c \
	prod_display.c \
	prod_disp_legend.c \
	anim.c \
	anim_fp.c \
	anim_opt.c \
	click.c\
	help.c \
	product_names.c \
	prod_select.c \
	packetselect.c \
	dispatcher.c \
	symbology_block.c \
	packet_1.c \
	packet_2.c \
	packet_3.c \
	packet_4.c \
	packet_5.c \
	packet_6.c \
	packet_7.c \
	packet_8.c \
	packet_9.c \
	packet_10.c \
	packet_11.c \
	packet_12.c \
	packet_15.c \
	packet_16.c \
	packet_17.c \
	packet_18.c \
	packet_19.c \
	packet_20.c \
	packet_23.c \
	packet_24.c \
	packet_25.c \
	packet_26.c \
	packet_28.c \
	packet_28area.c \
	packet_28radial.c \
	packet_0802.c \
	packet_0E03.c \
	packet_3501.c \
	radial_rle.c \
	raster.c \
	raster_digital.c \
	display_GAB.c \
	display_TAB.c \
	gif_output.c \
	png_output.c \
	cvg_map.c \
	overlay.c \
	cvg_xdr_infra.c \
	coord_conv.c \
	helpers.c

##THE FOLLOWING ARE FOR BUILD 12 CHANGES
##  CVG 8.7 new file prod_config.help
##          removed file prod_spec_info.help
HFILES = 	animation_opt.help \
		diskfile.help \
		fileselect.help \
		gifout.help \
		ilbselect.help \
		main.help \
		packetselect.help \
		pdlbselect.help \
		pngout.help \
		prefs.help \
		prod_config.help \
		screen.help \
		site_spec_info.help


##THE FOLLOWING ARE FOR BUILD 10 CHANGES
##  CVG 8.3 new file: hires_spw_2.lgd
##                  ntda_edc_008.lgd and ntda_edr_064.lgd 
##  CVG 8.4 new files: cc_064.lgd, hca.lgd, kdp.lgd, phi.lgd, quality.lgd,
##                     sdp.lgd, sdz.lgd, snr.lgd, zdr.lgd and 
##                     dpr_5v2.lgd, method_5_sample_no_flags.lgd, 
##                     method_5_sample_w_flags.lgd
##  CVG 8.5 new files: diff_test.lgd
##THE FOLLOWING ARE FOR BUILD 11 CHANGES
##  CVG 8.6 new files: hc.lgd
##          removed files: hca.lgd
##THE FOLLOWING ARE FOR BUILD 12 CHANGES
##  CVG 8.7 new files: dpr_5v3.lgd, hires_refl_5.lgd, hires_refl_5f.lgd
##  CVG 8.8 new files: hires_refl_5p.lgd
##  CVG 9.0 new files: phi_raw_5.lgd, zdr_raw_5.lgd
##  CVG 9.1 removed dpr_5v2.lgd
##  CVG 9.1 new files: daa_dsa_dua.lgd
##  CVG 9.1b new file: cc_raw_5.lgd

LFILES =	cc_064.lgd \
    cc_raw_5.lgd \
		daa_dsa_dua.lgd \
		diff_test.lgd \
		dpa_1.lgd \
		dpa_2.lgd \
		dpr_5v3.lgd \
		dsp1_1.lgd \
		dvil_2.lgd \
		hc.lgd \
		hires_refl_2.lgd \
		hires_refl_5.lgd \
		hires_refl_5f.lgd \
		hires_refl_5p.lgd \
		hires_vel1_2.lgd \
		hires_vel2_2.lgd \
		hires_spw_2.lgd \
		hreet.lgd \
		kdp.lgd \
		ntda_edc_008.lgd \
		ntda_edr_064.lgd \
		refl_1.lgd \
		refl_2.lgd \
		vel_1.lgd \
		vel_2.lgd \
		zdr.lgd \
		zdr_raw_5.lgd \
                zdr_raw_5_bias.lgd \
		phi.lgd \
		phi_raw_5.lgd \
		quality.lgd \
		sdp.lgd \
		sdz.lgd \
		snr.lgd \
		hhl.lgd \
		ihl.lgd \
		method_5_sample_no_flags.lgd \
		method_5_sample_w_flags.lgd


##THE FOLLOWING ARE FOR BUILD 10 CHANGES
##  CVG 8.3 new files: all_brown_16.plt, all_lt_gray_16.plt, hires_spw_2.plt,
##                      cvg_map.plt, ntda_edc_008.plt and ntda_edr_064.plt
##  CVG 8.4 new files: cc_16.plt, cc_256.plt, cc_64.plt, hc_16.plt, hca_256.plt, 
##                     kdp_16.plt, kdp_256.plt, quality_256.plt, snr.plt, 
##                     texture.plt, zdr_16.plt, zdr_256.plt, dpr_66v1.plt
##  CVG 8.5 new files  phi_64.plt AND ml_16.plt
##THE FOLLOWING ARE FOR BUILD 11 CHANGES
##  CVG 8.6 new files: hc_256.plt AND dp_precip_diff.plt
##          removed files: hca_256.plt
##THE FOLLOWING ARE FOR BUILD 12 CHANGES
##  CVG 9.0 new files: file: generic_method_5_86.plt, 
##                           vad_16.plt, zdr_raw_256.plt
##          removed file: vad_8.plt
##  CVG 9.1 new files: daa_dsa_dua.plt
##  CVG 9.1b removed files: cc_256_.plt

CFILES =	all_blue_16.plt \
		all_cyan_16.plt \
		all_gray_16.plt \
		all_lt_gray_16.plt \
		all_green_16.plt \
		all_magenta_16.plt \
		all_orange_16.plt \
		all_red_16.plt \
		all_brown_16.plt \
		all_white_16.plt \
		all_white_256.plt \
		all_white_64.plt \
		all_yellow_16.plt \
		cvg_map.plt \
		blank_16.plt \
		blank_256.plt \
		blank_32.plt \
		blank_64.plt \
		blank_8.plt \
		cc_16.plt \
		cc_64.plt \
		daa_dsa_dua.plt \
		dpa_rate_8.plt \
		dpr_66v1.plt \
		drefl_66.plt \
		dvil_255.plt \
		dvil_66.plt \
		echo_16.plt \
		et_16.plt \
		hail.plt \
		hc_16.plt \
		hc_256.plt \
		hires_refl.plt \
		hires_vel1.plt \
		hires_spw.plt \
		hreet.plt \
		kdp_16.plt \
		kdp_256.plt \
		mda.plt \
		meso.plt \
		meso_ru.plt \
		ml_16.plt \
		ntda_edc_008.plt \
		ntda_edr_064.plt \
		nws_64.plt \
		nws_66.plt \
		precip2_16.plt \
		precip_16.plt \
		rec_dop_12.plt \
		rec_refl_11.plt \
		refl_16.plt \
		refl_256a.plt \
		refl_64.plt \
		refl_8.plt \
		shear_16.plt \
		storm_id.plt \
		sw_8.plt \
		swp_4.plt \
		symbol_pkt2.plt \
		symbol_pkt20.plt \
		symbol_pkt28.plt \
		test_16.plt \
		text_1.plt \
		text_8.plt \
		tvs.plt \
		vad_16.plt \
		vel_16.plt \
		vel_64.plt \
		vel_65.plt \
		vel_66.plt \
		vel_8.plt \
		wind_5.plt \
		zdr_16.plt \
		zdr_256.plt \
		zdr_raw_256.plt \
                zdr_raw_256_bias.plt \
		quality_256.plt \
		snr.plt \
		hhl.plt \
		ihl.plt \
		texture.plt \
		dp_precip_diff.plt \
		phi_64.plt \
		generic_method_5_86.plt

TARGET = cvg

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

# Additional install of config and help files for cvg
##THE FOLLOWING ARE FOR BUILD 10 CHANGES
##  CVG 8.3 added file: rdr_loc.dat
##THE FOLLOWING ARE FOR BUILD 12 CHANGES
##  CVG 8.7 added file: prod_config
##          removed file: res_list
install::
	@if [ -d $(CVG_VER_DIR) ]; then set +x; \
		else (set -x; mkdir -p $(CVG_VER_DIR)); fi
	@if [ -d $(DEFAULT_CONF_DIR) ]; then set +x; \
		else (set -x; mkdir -p $(DEFAULT_CONF_DIR)); fi
	$(INSTALL) $(INSTDATFLAGS) config/prefs  $(DEFAULT_CONF_DIR)/prefs
	$(INSTALL) $(INSTDATFLAGS) config/radar_info  $(DEFAULT_CONF_DIR)/radar_info
	$(INSTALL) $(INSTDATFLAGS) config/prod_config  $(DEFAULT_CONF_DIR)/prod_config
	$(INSTALL) $(INSTDATFLAGS) config/resolutions  $(DEFAULT_CONF_DIR)/resolutions
	$(INSTALL) $(INSTDATFLAGS) config/site_data  $(DEFAULT_CONF_DIR)/site_data
	$(INSTALL) $(INSTDATFLAGS) config/prod_db_size  $(DEFAULT_CONF_DIR)/prod_db_size
	$(INSTALL) $(INSTDATFLAGS) config/sort_method  $(DEFAULT_CONF_DIR)/sort_method
	$(INSTALL) $(INSTDATFLAGS) config/prod_names  $(DEFAULT_CONF_DIR)/prod_names
	$(INSTALL) $(INSTDATFLAGS) config/descript_source  $(DEFAULT_CONF_DIR)/descript_source
	$(INSTALL) $(INSTDATFLAGS) config/rdr_loc.dat  $(DEFAULT_CONF_DIR)/rdr_loc.dat


	@if [ -d $(DEFAULT_CONF_DIR)/help ]; then set +x; \
		else (set -x; mkdir -p $(DEFAULT_CONF_DIR)/help); fi
	@for file in $(HFILES) ;\
		do \
		$(INSTALL) $(INSTDATFLAGS) config/help/$$file  $(DEFAULT_CONF_DIR)/help/$$file ;\
		done	


	@if [ -d $(DEFAULT_CONF_DIR)/colors ]; then set +x; \
		else (set -x; mkdir -p $(DEFAULT_CONF_DIR)/colors); fi
	$(INSTALL) $(INSTDATFLAGS) config/colors/palette_list  $(DEFAULT_CONF_DIR)/colors/palette_list
	@for file in $(CFILES) ;\
		do \
		$(INSTALL) $(INSTDATFLAGS) config/colors/$$file  $(DEFAULT_CONF_DIR)/colors/$$file ;\
		done


	@if [ -d $(DEFAULT_CONF_DIR)/legends ]; then set +x; \
		else (set -x; mkdir -p $(DEFAULT_CONF_DIR)/legends); fi
	@for file in $(LFILES) ;\
		do \
		$(INSTALL) $(INSTDATFLAGS) config/legends/$$file  $(DEFAULT_CONF_DIR)/legends/$$file ;\
		done	



-include $(DEPENDFILE)
