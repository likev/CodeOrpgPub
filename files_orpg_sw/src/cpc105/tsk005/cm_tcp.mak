# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/05/18 18:45:04 $
# $Id: cm_tcp.mak,v 1.22 2011/05/18 18:45:04 ccalvert Exp $
# $Revision: 1.22 $
# $State: Exp $
#
# 12MAR2002 Chris Gilbert - NA01-34801 Issue 1-886 - Add support for TCP Dial-out.
# 06FEB2003 Chris Gilbert - Issues  2-129 and 2-077 - Add static links for OPUP and clean libs.
#

CM_COMMON_DIR = ../tsk004
include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)
include $(CM_COMMON_DIR)/make.cm_common

LOCAL_INCLUDES = -I$(CM_COMMON_DIR) 

# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.
LOCAL_DEFINES = -DCM_TCP -DSNMP

# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) $(SYS_INCLUDES)

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES) $(SYS_DEFINES)

# If extra library paths are needed for this specific module.
LIBPATH = $(SYS_LIBPATH)

# Removed reference to simpact freeway library 6/28/01
# Define libraries unique for the different architectures
slrs_spk_LIBRARIES = -lnsl -lsocket -lelf -lrt -lm

lnux_x86_LIBRARIES = -lnsl -lrt -ldl -lm

LOCAL_LIBRARIES = -linfr $($(ARCH)_LIBRARIES)

# Different order/set of libraries needed to build with debug information
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES) -lbzip2
GPROF_LOCALLIBS = $(DEBUG_LOCALLIBS)

PURE_LOCALLIBS = $(DEBUG_LOCALLIBS)
QUAN_LOCALLIBS = $(DEBUG_LOCALLIBS)
PRCOV_LOCALLIBS = $(DEBUG_LOCALLIBS)

EXTRA_LIBRARIES = 

# Architecture and debug or profiling tool dependent linker options for
# slrs_spk
slrs_spk_DEBUG_LD_OPTS = $(slrs_spk_LD_OPTS)
slrs_spk_GPROF_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)

slrs_spk_PURE_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)
slrs_spk_QUAN_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)
slrs_spk_PRCOV_LD_OPTS = $(slrs_spk_DEBUG_LD_OPTS)

# Architecture and debug or profiling tool dependent linker options for
# lnux_x86
lnux_x86_DEBUG_LD_OPTS =
lnux_x86_GPROF_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)

lnux_x86_PURE_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)
lnux_x86_QUAN_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)
lnux_x86_PRCOV_LD_OPTS = $(lnux_x86_DEBUG_LD_OPTS)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
GPROFCCFLAGS = $(COMMON_GPROFCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

PURECCFLAGS = $(COMMON_PURECCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
QUANCCFLAGS = $(COMMON_QUANCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
PRCOVCCFLAGS = $(COMMON_PRCOVCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

DEPENDFLAGS =

SRCS =	cmt_main.c		\
	cmt_config.c		\
	cmt_shared.c		\
	cmt_login.c		\
	cmt_sock.c		\
	cmt_snmp.c		\
	cmt_tcp.c		\
	cmt_dial.c		\
	$(CM_COMMON_SRCS)

TARGET = cm_tcp

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.cbin

install::
	ln $(MAKETOP)/bin/$(ARCH)/cm_tcp $(TOOLSDIR)/cm_tcp1

-include $(DEPENDFILE)
