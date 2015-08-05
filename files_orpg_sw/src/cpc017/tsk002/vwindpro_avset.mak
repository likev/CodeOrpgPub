# RCS info
# $Author: cmn $
# $Locker:  $
# $Date: 2008/08/26 16:14:47 $
# $Id: vwindpro_avset.mak,v 1.1 2008/08/26 16:14:47 cmn Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# In case if there are any includes that are not in standard places. 
# Normally this should always be blank.
LOCAL_INCLUDES =
# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

# In case if there are any local defines.
LOCAL_DEFINES = -DAVSET
# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# This list has all the includes that are needed for the compile.
# Local copies of a file are given preference over the system 
# location.
# Append $(X_INCLUDES) when appropriate ...
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
                         $(LOCAL_INCLUDES)

# This is a list of all defines.
# Append $(X_DEFINES) when appropriate ...
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)


CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
PURECCFLAGS = $(COMMON_PURECCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)

# use following for makefile-specific makedepend flags
# (re: SYS_DEPENDFLAGS in make.$(ARCH)
# We do no currently makedepend Fortran source files
DEPENDFLAGS =

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

FSRCS =	vwindpro.ftn \
	a317a2.ftn \
	a317b2.ftn \
	a317c2.ftn \
	a317d2.ftn \
	a317e2.ftn \
	a317f2.ftn \
	a317g2.ftn \
	a317h2.ftn \
	a317i2.ftn \
	a317j2.ftn \
        a317k2.ftn \
	a317l2.ftn \
	a317n2.ftn \
	a317o2.ftn \
	a317p2.ftn \
	a317u2.ftn \
	a317v2.ftn \
	a317z2.ftn \
	a31831.ftn \
	a31832.ftn \
	a31833.ftn \
	a31834.ftn \
	a31835.ftn \
	a31836.ftn \
	a31837.ftn \
	a31838.ftn \
	a31839.ftn \
	a3183a.ftn \
	a3183b.ftn \
	a3183c.ftn \
	a3183d.ftn \
	a3183e.ftn \
	a3183f.ftn \
	a3183g.ftn

CSRCS = a317y2.c

# Following is for specifying any non-local object files (e.g., cpc-level
# object files)
ADDITIONAL_OBJS =

TARGET = a_vwindpro

include $(MAKEINC)/make.fbin
