# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:06:21 $
# $Id: libaprcom.mak,v 1.4 2014/03/07 19:06:21 steves Exp $
# $Revision: 1.4 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# In case if there are any includes that are not in standard places. 
# Normally this should always be blank.
FC_LOCAL_INCLUDES = 
FPP_LOCAL_INCLUDES = 

# In case if there are any local fortran defines.
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


FCFLAGS = $(COMMON_FCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
DEBUGFCFLAGS = $(COMMON_DEBUGFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
FPPFLAGS = $(COMMON_FPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEBUGFPPFLAGS = $(COMMON_DEBUGFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)

# We cannot "makedepend" Fortran source files ...
DEPENDFILE =

LIB_FSRCS = a30740.ftn \
	a30744.ftn \
	a30745.ftn \
	a30746.ftn \
	a30748.ftn \
	a30749.ftn \
	a3074a.ftn \
	a31483.ftn \
	a31484.ftn \
	a31485.ftn \
	a31486.ftn \
	a31487.ftn \
	a31488.ftn \
	a3148a.ftn \
	a3148b.ftn \
	a3148c.ftn \
	a3148e.ftn \
	a3148f.ftn \
	a3148h.ftn 
        

LIB_TARGET = aprcom

clean::
	$(RM) $(ARCH)/*.f

include $(MAKEINC)/make.cflib
