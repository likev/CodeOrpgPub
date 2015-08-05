# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2004/02/05 23:04:26 $
# $Id: libtda.mak,v 1.2 2004/02/05 23:04:26 ccalvert Exp $
# $Revision: 1.2 $
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

FC_slrs_spk_LDFLAGS =
FC_slrs_x86_LDFLAGS =
FC_hpux_rsk_LDFLAGS = +U77

FCFLAGS = $(COMMON_FCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
PUREFCFLAGS = $(COMMON_PUREFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
DEBUGFCFLAGS = $(COMMON_DEBUGFCFLAGS) $(FC_ALL_INCLUDES) $(FC_ALL_DEFINES)
FPPFLAGS = $(COMMON_FPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
PUREFPPFLAGS = $(COMMON_PUREFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)
DEBUGFPPFLAGS = $(COMMON_DEBUGFPPFLAGS) $(FPP_ALL_INCLUDES) $(FPP_ALL_DEFINES)

# We cannot "makedepend" Fortran source files ...
DEPENDFILE =

LIB_FSRCS = a317b9.ftn \
	a317d8.ftn \
	a317e9.ftn \
	a317f8.ftn \
	a317f9.ftn \
	a317g8.ftn \
	a317g9.ftn \
	a317h8.ftn \
	a317h9.ftn \
	a317i8.ftn \
	a317i9.ftn \
	a317j8.ftn \
	a317k8.ftn \
	a317k9.ftn \
	a317l8.ftn \
	a317l9.ftn \
	a317n8.ftn \
	a317o8.ftn \
	a317p8.ftn \
	a317q8.ftn \
	a317q9.ftn \
	a317s8.ftn \
	a317s9.ftn \
	a317t9.ftn \
	a317u8.ftn \
	a317w8.ftn \
	a317y9.ftn \
	a317z8.ftn
        

LIB_TARGET = tda

clean::
	$(RM) $(ARCH)/*.f

include $(MAKEINC)/make.cflib
