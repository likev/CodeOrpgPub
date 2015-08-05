# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2007/01/25 18:46:45 $
# $Id: cpc100_lib005.make,v 1.7 2007/01/25 18:46:45 aamirn Exp $
# $Revision: 1.7 $
# $State: Exp $

# This is the parent makefile for Remote Tool Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = librmt.mak \
		librmtt.mak 

include $(MAKEINC)/make.parent_lib

ifeq ($(OPUP_BLD),yes)
BINMAKEFILES =
else
BINMAKEFILES =	rmt_passwd.mak
endif

include $(MAKEINC)/make.parent_bin

