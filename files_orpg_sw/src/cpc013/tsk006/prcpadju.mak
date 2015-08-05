# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 18:54:57 $
# $Id: prcpadju.mak,v 1.5 2014/03/07 18:54:57 steves Exp $
# $Revision: 1.5 $
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
ALL_INCLUDES = $(STD_INCLUDES) $(LOCALTOP_INCLUDES) $(TOP_INCLUDES) \
			 $(LOCAL_INCLUDES) 

# This is a list of all defines.
ALL_DEFINES = $(STD_DEFINES) $(OS_DEFINES) $(LOCAL_DEFINES)

CCFLAGS = $(COMMON_CCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
DEBUGCCFLAGS = $(COMMON_DEBUGCCFLAGS) $(ALL_INCLUDES) $(ALL_DEFINES)
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

FSRCS =  prcpadju.ftn \
	prcgagsc.ftn \
	a31360.ftn \
	a31361.ftn \
	a31362.ftn \
	a3136k.ftn \
	a3136r.ftn \
	a3136t.ftn \
	a3136w.ftn \
	a3136l.ftn \
	a3136n.ftn \
	a3136p.ftn \
	a3136q.ftn \
	a3136u.ftn \
	a3136v.ftn \
        a3136x.ftn 
         

CSRCS = prcpadju_copy_input.c \
	prcpadju_set_in_out.c \
	prcpadju_report_error.c

TARGET = prcpadju

include $(MAKEINC)/make.fbin

