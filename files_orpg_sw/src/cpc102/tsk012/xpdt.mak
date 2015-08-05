# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:28:54 $
# $Id: xpdt.mak,v 1.26 2014/03/07 19:28:54 steves Exp $
# $Revision: 1.26 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES =  -I$(LOCALTOP)/src/cpc108/tsk011 -I$(MAKETOP)/src/cpc101/lib003 -I$(LOCALTOP)/src/cpc001 -I$(MAKETOP)/src/cpc001

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

LOCAL_DEFINES = -DNO_MAIN_FUNCT 

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(X_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(X_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH = $(X_LIBPATH)

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
LOCAL_LIBRARIES = -Wl,-Bstatic -lorpg -linfr -Wl,-Bdynamic -lz -lbz2 -lcrypt

# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES = 
X_LIBS = -lXm -lXt -lX11 -lMrm

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS = -lm 

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)



CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
DEPENDFLAGS =

SRCS =  decode.prod.c 				\
	display_dhr_data.c			\
	display_graphics_attributes_table.c	\
	display_meso_data.c			\
	display_probability_of_hail_data.c	\
	display_product_attributes.c		\
	display_radial_data.c			\
	display_raster_data.c			\
	display_hires_raster_data.c		\
	display_storm_track_data.c		\
	display_swp_data.c			\
	display_tabular_data.c			\
	display_tvs_data.c			\
	display_vad_data.c			\
	earth_to_radar_coords_proc.c		\
	find_best_color.c     			\
	make_windbarb.c				\
	overlay_USGS_GRV_file.c			\
	select_hail_thresholds.c		\
	select_storm_track_cells.c		\
	xpdt_main.c				\
	orpgpat.c				\
	decompress.c				\
	xpdt_query.c

ADDITIONAL_OBJS =

TARGET = xpdt

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)




