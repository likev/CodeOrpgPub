# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2004/02/05 22:58:41 $
# $Id: trfrcalg.mak,v 1.11 2004/02/05 22:58:41 ccalvert Exp $
# $Revision: 1.11 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# In case if there are any includes that are not in standard places. 
# Normally this should always be blank.
FC_LOCAL_INCLUDES = 
FPP_LOCAL_INCLUDES = 
# In case if there are any local fortran defines.
FC_LOCAL_DEFINES =
FPP_LOCAL_DEFINES =

# This list has all the includes that are needed for the compile.
# Local copies of a file are given preference over the system 
# location.
FC_ALL_INCLUDES = $(FC_STD_INCLUDES) $(FC_LOCALTOP_INCLUDES) \
						$(FC_TOP_INCLUDES) $(FC_LOCAL_INCLUDES)
FPP_ALL_INCLUDES = $(FPP_STD_INCLUDES) $(FPP_LOCALTOP_INCLUDES) \
						$(FPP_TOP_INCLUDES) $(FPP_LOCAL_INCLUDES)

# This is a list of all fortran defines.
FC_ALL_DEFINES = $(FC_STD_DEFINES) $(FC_OS_DEFINES) $(FC_LOCAL_DEFINES)
FPP_ALL_DEFINES = $(FPP_STD_DEFINES) $(FPP_OS_DEFINES) $(FPP_LOCAL_DEFINES)

# If extra library paths are needed for this specific module.
FC_LIBPATH =

# These libraries are named the same on all architerctures at this time.
# Location of each of libraries depends on the architecture. e.g. ORPG HP 
# libraries are located in $(TOP)/lib/hpux_rsk. If there are separate 
# library names for different architectures, the below portion of the makefile 
# will have to be moved to $(MAKEINC)/make.$(ARCH).
# libraries specific to orpg
FC_LOCAL_LIBRARIES =  $(F_ALGORITHM_LIBS)
FC_PURE_LOCALLIBS = $(FC_LOCAL_LIBRARIES) -lbzip2
FC_DEBUG_LOCALLIBS = $(FC_LOCAL_LIBRARIES) -lbzip2
FC_EXTRA_LIBRARIES = -lV3

# architecture specific system libraries and load flags.
FC_slrs_spk_LIBS = $(F_ALGORITHM_SYS_LIBS)
FC_slrs_x86_LIBS = $(F_ALGORITHM_SYS_LIBS)
FC_lnux_x86_LIBS = $(F_ALGORITHM_SYS_LIBS)
FC_hpux_rsk_LIBS = -lV3
FC_slrs_spk_LDFLAGS =
FC_slrs_x86_LDFLAGS =
FC_hpux_rsk_LDFLAGS = +U77

# some system libraries, if needed.
FC_SYS_LIBS = 

FCFLAGS = $(COMMON_FCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
PUREFCFLAGS = $(COMMON_PUREFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
DEBUGFCFLAGS = $(COMMON_DEBUGFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
FPPFLAGS = $(COMMON_FPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
PUREFPPFLAGS = $(COMMON_PUREFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEBUGFPPFLAGS = $(COMMON_DEBUGFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)

# We cannot "makedepend" Fortran source files ... it is important to define
# DEPENDFILE to be "empty" (protects the depend command lines)
#DEPENDFLAGS = 
DEPENDFILE =

FSRCS =	trfrcalg.ftn \
	a315a7.ftn \
	a315b6.ftn \
	a315e7.ftn \
	a315f7.ftn \
	a315g7.ftn \
	a315h7.ftn \
	a315i6.ftn \
	a315i7.ftn \
	a315j6.ftn \
	a315j7.ftn \
	a315k6.ftn \
	a315l6.ftn \
	a315o7.ftn \
	a315q6.ftn \
	a315s7.ftn \
	a315x6.ftn \
	a315y7.ftn \
	a315z7.ftn

# Following is for specifying any non-local object files (e.g., cpc-level
# object files)
ADDITIONAL_OBJS =

TARGET = trfrcalg

include $(MAKEINC)/make.fbin





