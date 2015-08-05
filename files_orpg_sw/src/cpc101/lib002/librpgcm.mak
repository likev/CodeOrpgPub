# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 19:06:21 $
# $Id: librpgcm.mak,v 1.11 2014/03/07 19:06:21 steves Exp $
# $Revision: 1.11 $
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

LIB_FSRCS = a3cm01.ftn \
	a3cm02.ftn \
	a3cm08.ftn \
	a3cm09.ftn \
	a3cm15.ftn \
	a3cm16.ftn \
	a3cm17.ftn \
	a3cm20.ftn \
	a3cm22.ftn \
	a3cm25.ftn \
	a3cm27.ftn \
	a3cm28.ftn \
	a3cm29.ftn \
	a3cm30.ftn \
	a3cm31.ftn \
	a3cm32.ftn \
	a3cm33.ftn \
	a3cm36.ftn \
	a3cm37.ftn \
	a3cm38.ftn \
	a3cm39.ftn \
	a3cm3a.ftn \
	a3cm3b.ftn \
	a3cm3c.ftn \
	a3cm60.ftn \
	a3cm70.ftn \
	t41192.ftn \
	t41193.ftn \
	t41194.ftn

LIB_TARGET = rpgcm

clean::
	$(RM) $(ARCH)/*.f

include $(MAKEINC)/make.cflib
