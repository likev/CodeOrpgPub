# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2007/01/25 18:46:38 $
# $Id: cpc100_lib006.make,v 1.14 2007/01/25 18:46:38 aamirn Exp $
# $Revision: 1.14 $
# $State: Exp $

# This is the parent makefile for Remote System Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES =	librss.mak \
		librsst.mak 

include $(MAKEINC)/make.parent_lib

ifeq ($(OPUP_BLD),yes)
BINMAKEFILES =	rssd.mak
else
BINMAKEFILES =	rssd.mak \
                rssdt.mak
endif

include $(MAKEINC)/make.parent_bin

