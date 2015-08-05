# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:34:01 $
# $Id: nbtcp.mak,v 1.10 2014/03/07 19:34:01 steves Exp $
# $Revision: 1.10 $
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
# NOTE: Do Not Remove This Static Linker Option (leave the system libs dynamic)
LOCAL_LIBRARIES = $(MAKETOP)/lib/$(ARCH)/libinfr.a -lz -lbz2
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES) 

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_LD_OPTS =

lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)


GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)


DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
EXTRA_LIBRARIES = 

CCFLAGS = -g $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)


DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

SRCS =	nbtcp.c \
    	nbtcp_cmtcp.c \
	nbtcp_build_master_file.c \
	nbtcp_process_rpg_msgs.c \
        nbtcp_wmo_awips_hdr.c \
	nbtcp_read_product_list.c \
	nbtcp_send_messages.c \
	nbtcp_socket_mgr.c \
	nbtcp_smipp.c \
	nbtcp_terminal.c

TARGET = nbtcp

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.tool

-include $(DEPENDFILE)
