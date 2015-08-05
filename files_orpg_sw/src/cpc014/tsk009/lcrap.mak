# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:57:12 $
# $Id: lcrap.mak,v 1.14 2014/03/07 18:57:12 steves Exp $
# $Revision: 1.14 $
# $State: Exp $

# Template make description file for describing a Fortran binary file

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
FC_LOCAL_LIBRARIES =  -laprcom $(F_ALGORITHM_LIBS)
FC_DEBUG_LOCALLIBS = $(FC_LOCAL_LIBRARIES)
FC_EXTRA_LIBRARIES = -lV3

# architecture specific system libraries and load flags.

# some system libraries, if needed.
FC_SYS_LIBS = 

FCFLAGS = $(COMMON_FCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
DEBUGFCFLAGS = $(COMMON_DEBUGFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
FPPFLAGS = $(COMMON_FPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEBUGFPPFLAGS = $(COMMON_DEBUGFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)

# We cannot "makedepend" Fortran source files ... it is important to define
# DEPENDFILE to be "empty" (protects the depend command lines)
#DEPENDFLAGS = 
DEPENDFILE =

FSRCS =	lcrap.ftn \
	a3149a.ftn \
	a3149b.ftn \
	a3149d.ftn \
	a3149f.ftn \
	a3149g.ftn \
	a3149i.ftn \
	a3149q.ftn \
	a3149s.ftn \
	a3149t.ftn  

# Following is for specifying any non-local object files (e.g., cpc-level
# object files)
ADDITIONAL_OBJS =

TARGET = lcrap

include $(MAKEINC)/make.fbin




