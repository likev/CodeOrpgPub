# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:31:53 $
# $Id: rdasim_simulator.mak,v 1.21 2014/03/07 19:31:53 steves Exp $
# $Revision: 1.21 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LOCAL_INCLUDES =  -I

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
LOCAL_LIBRARIES = -lorpg -linfr
DEBUG_LOCALLIBS = $(LOCAL_LIBRARIES)
EXTRA_LIBRARIES = 

# architecture specific system libraries.
lnux_x86_LD_OPTS = -lm
# some system libraries, if needed.
SYS_LIBS = -lm -lrt
X_LIBS = 

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEPENDFLAGS =

clean:: rdasim_clean
install:: rdasim_smi
all:: rdasim_smi
rdasim_smi: rdasim_smi.h
	smipp -o "-I. -I$(MAKETOP)/lib/include -I$(MAKETOP)/include -DCALLED_FROM_SMIPP" rdasim_smi.h

SRCS =	rdasim_main.c \
	rdasim_construct_radials.c \
	rdasim_driver.c  \
	rdasim_process_requests.c \
	rdasim_smi.c \
	rdasim_rda_rpg_msgs.c \
	rdasim_read_requests.c

TARGET = rda_simulator

DEPENDFILE = ./depend.$(TARGET).$(ARCH) rdasim_smi.c
#DEPENDFLAGS = -f $(DEPENDFILE)

rdasim_clean:
	rm -f  rdasim_smi.c
	touch rdasim_smi.c



include $(MAKEINC)/make.tool

#-include $(DEPENDFILE)
