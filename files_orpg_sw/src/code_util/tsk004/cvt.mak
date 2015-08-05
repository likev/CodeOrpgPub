# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:38:14 $
# $Id: cvt.mak,v 1.12 2014/03/07 19:38:14 steves Exp $
# $Revision: 1.12 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


# CVT 4.4
CVT_DECODE_CONFIG = $(MAKETOP)/.cvt




LOCAL_INCLUDES =

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.


ifdef STANDALONE_CVT
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
LIBPATH = -L/usr/local/lib

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg

## FOR CODE: the libbzip2 would have to be added for all Solaris platforms.

## FOR CVT 4.4.1: No longer link static bzip library for stand-alone.

LOCAL_LIBRARIES = -lbz2 -linfr


ifdef STANDALONE_CVT

LOCAL_LIBRARIES = -lbz2  $(MAKETOP)/lib/$(ARCH)/libinfr.a

endif


# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


EXTRA_LIBRARIES =

X_LIBS =

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS =

lnux_x86_DEBUG_LD_OPTS = -lm -ldl
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)



CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEPENDFLAGS =

# CVT 4.4 - added decode_level.c, product_load.c, and help_functions.c
SRCS =          cvt.c \
                bscan_format.c \
                cvt_dispatcher.c \
                product_load.c \
                decode_level.c \
                inventory_functions.c \
                message_header.c \
                misc_functions.c \
                help_functions.c \
                orpg_header.c \
                cvt_display_GAB.c \
                cvt_display_TAB.c \
                display_SATAP.c \
                cvt_packet_1.c \
                cvt_packet_2.c \
                cvt_packet_3.c \
                cvt_packet_4.c \
                cvt_packet_5.c \
                cvt_packet_6.c \
                cvt_packet_7.c \
                cvt_packet_8.c \
                cvt_packet_9.c \
                cvt_packet_10.c \
                cvt_packet_11.c \
                cvt_packet_12.c \
                cvt_packet_13.c \
                cvt_packet_14.c \
                cvt_packet_15.c \
                cvt_packet_16.c \
                cvt_packet_17.c \
                cvt_packet_18.c \
                cvt_packet_19.c \
                cvt_packet_20.c \
                cvt_packet_23.c \
                cvt_packet_24.c \
                cvt_packet_25.c \
                cvt_packet_26.c \
                cvt_packet_27.c \
                cvt_packet_28.c \
                cvt_packet_0802.c \
                cvt_packet_0E03.c \
                cvt_packet_3501.c \
                cvt_xdr_infra.c \
                packet_AF1F.c \
                packet_BA07.c \
                summary_info.c \
                sym_block.c \
                time_functions.c

ADDITIONAL_OBJS =

TARGET = cvt

DEPENDFILE = ./depend.$(TARGET).$(ARCH)


include $(MAKEINC)/make.tool


# CVT 4.4
# Additional install of decode configuration file for cvt
# Currently one sample configuration file is installed
install::
	@if [ -d $(CVT_DECODE_CONFIG) ]; then set +x; \
		else (set -x; mkdir -p $(CVT_DECODE_CONFIG)); fi
	$(INSTALL) $(INSTDATFLAGS) config/decode_params.1992  $(CVT_DECODE_CONFIG)/decode_params.1992








-include $(DEPENDFILE)
