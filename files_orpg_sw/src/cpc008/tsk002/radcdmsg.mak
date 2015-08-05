# RCS info
# $Author: jeffs $
# $Locker:  $
# $Date: 2014/03/19 17:52:17 $
# $Id: radcdmsg.mak,v 1.16 2014/03/19 17:52:17 jeffs Exp $
# $Revision: 1.16 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# In case if there are any includes that are not in standard places. 
# Normally this should always be blank.
LOCAL_INCLUDES = 
# You can also include architecture specific includes, if needed, by
# defining $(ARCH)_INC and then adding it to the list of ALL_INCLUDES.

# In case if there are any local defines.
LOCAL_DEFINES =
# You can also include architecture specific defines, if needed, by
# defining $(ARCH)_DEF and then adding it to the list of ALL_DEFINES.

# This list has all the includes that are needed for the compile.
# Local copies of a file are given preference over the system 
# location.
# Append $(X_INCLUDES) when appropriate ...
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# During development and testing you may want to use local copies of a
# library. For this you need uncomment the following variable. NOTE:
# THIS SHOULD ONLY BE UNCOMMENTED FOR DEVELOPMENT AND TESTING, NOT
# FOR OPERATIONAL USE.
#FCLDOPTIONS = -L$(LOCALTOP)/lib/$(ARCH)

# This is a list of all defines.
# Append $(X_DEFINES) when appropriate ...
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)


CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
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
FC_ALL_DEFINES = $(FC_STD_DEFINES) $(FC_OS_DEFINES) $(FC_XOPEN_DEFINES) \
						$(FC_LOCAL_DEFINES)
FPP_ALL_DEFINES = $(FPP_STD_DEFINES) $(FPP_OS_DEFINES) $(FPP_XOPEN_DEFINES) \
						$(FPP_LOCAL_DEFINES)

# If extra library paths are needed for this specific module.
# Specify $(X_LIBPATH) when appropriate ...
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

FSRCS =	radcdmsg.ftn \
	a30821.ftn \
	a30822.ftn \
	a30823.ftn \
	a30824.ftn \
	a30825.ftn \
	a30826.ftn \
	a30828.ftn \
	a30829.ftn \
	a3082a.ftn \
	a3082b.ftn \
	a3082c.ftn \
	a3082d.ftn \
	a3082e.ftn \
	a3082f.ftn \
	a3082g.ftn \
	a3082h.ftn \
	a3082i.ftn \
	a3082j.ftn \
	a3082k.ftn \
	a3082l.ftn \
	a3082m.ftn \
	a3082n.ftn \
	a3082o.ftn \
	a3082p.ftn \
	a3082q.ftn \
	a3082s.ftn \
	a3082t.ftn \
	a3082u.ftn \
	a3082v.ftn

CSRCS = radcdmsg_read_rdastatus_lb.c rcm_convert_rcm.c

ADDITIONAL_OBJS =

TARGET = radcdmsg

DEPENDFILE = ./depend.$(TARGET).$(ARCH)

include $(MAKEINC)/make.fbin

