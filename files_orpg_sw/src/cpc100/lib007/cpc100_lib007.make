# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2007/01/25 18:46:28 $
# $Id: cpc100_lib007.make,v 1.5 2007/01/25 18:46:28 aamirn Exp $
# $Revision: 1.5 $
# $State: Exp $

# This is the parent makefile for Supplemental Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = libsupp.mak

include $(MAKEINC)/make.parent_lib

ifeq ($(OPUP_BLD),yes)
BINMAKEFILES =
else
BINMAKEFILES = le_vl.mak
endif

include $(MAKEINC)/make.parent_bin

