# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:54:58 $
# $Id: prcprtac.mak,v 1.6 2014/03/07 18:54:58 steves Exp $
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

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

# If extra library paths are needed for this specific function.
LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate
# library names for different architectures, the below portion of the makefile
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = $(C_ALGORITHM_LIBS)

# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES =

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = 

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)


# Flags to be passed to compiler
CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =

SRCS	=	prcprtac_main.c \
                prcprtac_file_io.c \
		prcprate_initialize_file_io.c \
                prcprate_initialize_polar2lfm_tbls.c \
                prcprate_build_lfm_lookup.c \
                prcprate_find_holes.c \
                prcprate_lfm_to_azran.c \
		prcprate_RateAlg_buffctrl.c \
		prcprate_copy_input_buffer.c \
                prcprate_rate_scan_ctrl.c \
		prcprate_init_precip_table.c \
		prcprate_fill_precip_table.c \
		prcprate_init_rate_adapt.c \
		prcprate_read_header_recd.c \
		prcprate_avg_hybscn_pairs.c \
		prcprate_update_bad_scans.c \
		prcprate_range_effect_correct.c \
		prcprate_lfm4_map.c \
		prcpacum_AcumAlg_buffctrl.c \
		prcpacum_init_acum_adapt.c \
		prcpacum_copy_in2out_buffer.c \
		prcpacum_fill_supl_array.c \
		prcpacum_write_rate_hdr.c \
		prcpacum_set_up_hdrs.c \
		prcpacum_read_header_flds.c \
		prcpacum_fill_new_hdrs.c \
		prcpacum_define_hour.c \
		prcpacum_convert_time.c \
		prcpacum_normalize_times.c \
		prcpacum_time_in_hour.c \
		prcpacum_buffer_needs.c \
		prcpacum_fill_supl_missing.c \
		prcpacum_restore_times.c \
                prcpacum_add_prev_hrly.c \
                prcpacum_add_scans.c \
                prcpacum_determine_hourly_acum.c \
                prcpacum_determine_hourly_method.c \
                prcpacum_determine_period_acums.c \
                prcpacum_extrap_acum.c \
                prcpacum_hrly_outli_corr.c \
                prcpacum_interp1_acum.c \
                prcpacum_interp2_acum.c \
                prcpacum_max_hrly_val.c \
                prcpacum_new_hrly_contrib.c \
                prcpacum_prepare_outputs.c \
                prcpacum_scan_to_scan.c \
                prcpacum_subtract_prev_hrly.c \
                prcpacum_subtract_scans.c \
                prcpacum_test_n_interp.c \
                prcpacum_write_hdr_fields.c \
                prcpacum_write_scans.c

TARGET = prcprtac

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

-include $(DEPENDFILE)
