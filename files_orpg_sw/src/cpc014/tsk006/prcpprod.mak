# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:57:13 $
# $Id: prcpprod.mak,v 1.7 2014/03/07 18:57:13 steves Exp $
# $Revision: 1.7 $
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
FC_ALL_INCLUDES = $(FC_STD_INCLUDES) $(FC_TOP_INCLUDES) $(FC_LOCAL_INCLUDES)
FPP_ALL_INCLUDES = $(FPP_STD_INCLUDES) $(FPP_TOP_INCLUDES) $(FPP_LOCAL_INCLUDES)

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
FC_DEBUG_LOCALLIBS = $(FC_LOCAL_LIBRARIES)
FC_EXTRA_LIBRARIES = -lV3

# architecture specific system libraries and load flags.

# some system libraries, if needed.
FC_SYS_LIBS = 

FCFLAGS = $(COMMON_FCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
DEBUGFCFLAGS = $(COMMON_DEBUGFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
FPPFLAGS = $(COMMON_FPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEBUGFPPFLAGS = $(COMMON_DEBUGFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEPENDFLAGS = 

# We cannot "makedepend" Fortran source files ...
DEPENDFILE =

FSRCS =	prcpprod.ftn \
	prcpprod_cd07_updt.ftn \
	a31461.ftn \
	a31463.ftn \
	a31464.ftn \
	a31465.ftn \
	a31466.ftn \
	a31467.ftn \
	a31468.ftn \
	a3146a.ftn \
	a3146b.ftn \
	a3146e.ftn \
	a3146f.ftn \
	a3146g.ftn \
	a3146h.ftn \
	a3146i.ftn \
	a3146j.ftn \
	a3146k.ftn \
	a3146m.ftn \
	a3146o.ftn \
	a3146q.ftn \
	a3146r.ftn \
	a3146s.ftn \
	a3146t.ftn \
	a3146u.ftn \
	a3146v.ftn \
	a3146w.ftn \
	a3146z.ftn \
	a31471.ftn \
	a31472.ftn \
	a31473.ftn \
	a31474.ftn \
	a31475.ftn \
	a31476.ftn \
	a31482.ftn \
	a31489.ftn \
	a3148a.ftn \
	a3148d.ftn \
	a3148n.ftn \
	a3148p.ftn \
	a3148t.ftn \
	a3148u.ftn \
	a3148v.ftn \
	a3148x.ftn \
	a31491.ftn \
	a31492.ftn \
	a31493.ftn \
	a31494.ftn \
	a31495.ftn

ADDITIONAL_OBJS = ../$(ARCH)/a31478.o \
		../$(ARCH)/a3148l.o

TARGET = prcpprod

include $(MAKEINC)/make.fbin

