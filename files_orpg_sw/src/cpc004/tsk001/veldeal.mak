# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:51:27 $
# $Id: veldeal.mak,v 1.21 2014/03/07 18:51:27 steves Exp $
# $Revision: 1.21 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LOCAL_INCLUDES =

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DRPGC_LIBRARY

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

# If extra library paths are needed for this specific module.
# Specify $(X_LIBPATH) when appropriate ...
LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate
# library names for different architectures, the below portion of the makefile
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = $(C_ALGORITHM_LIBS) -lpthread

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

SRCS	= veldeal.c \
	a304d2.c \
        a304d4.c \
        a304d8.c \
	a304da.c \
	a304db.c \
        a304dc.c \
	a304di.c \
        a304dv.c \
	mpda_buf_cntrl.c \
	update_ewt_status.c \
	check_for_mpda_vcp.c \
	build_lookup_table.c \
        initialize_data_arrays.c \
	initialize_lookup_tables.c \
        save_mpda_data.c \
	save_ref_data.c \
	fix_nyq_value.c \
        apply_mpda.c \
 	range_unfold_prf_scans.c \
	get_closest_radial.c \
        rng_unf_chks_ok.c \
	fix_trip_marks.c \
	assign_vel_trip.c \
        get_trip_num.c \
	align_prf_scans.c \
        process_mpda_scans.c \
	fix_radial_errors.c \
        check_seed_unf.c \
	initialize_unf_seeds.c \
	fix_azimuthal_errors.c \
        get_azm_avgs.c \
	unfold_vel.c \
	get_derived_params.c \
        despeckle_results.c \
	first_triplet_attempt.c \
	check_unf_diffs.c \
        initialize_rad_direction.c \
	get_seed_value.c \
	multi_prf_unf.c \
        get_ewt_value.c \
	second_triplet_attempt.c \
	vel_short_to_UIF.c \
        pairs_and_trips_attempts.c \
	get_lookup_table_value.c \
        final_unf_attempts.c \
	replace_orig_vals.c \
	fill_radials.c \
        build_ewt_struct.c \
	vcp_setup.c \
	build_rad_header.c \
        output_mpda_data.c \
	build_rng_unf_arrys.c \
	initialize_uif_table.c \
        get_moments_status.c \
	get_adapt_params.c \
        vdeal_2d.c \
        vdeal_banbks.c \
        vdeal_clump.c \
        vdeal_dealiase.c \
        vdeal_estimate_ew.c \
        vdeal_ew.c \
        vdeal_realtime.c \
        vdeal_vad.c \
	vdeal_test.c \
	vdeal_preprocess.c \
	vdeal_analyze.c \
	vdeal_cleanup_data.c


TARGET = veldeal

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

-include $(DEPENDFILE)
