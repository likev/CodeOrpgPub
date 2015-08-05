# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2007/01/25 18:46:53 $
# $Id: cpc100_lib003.make,v 1.9 2007/01/25 18:46:53 aamirn Exp $
# $Revision: 1.9 $
# $State: Exp $

# This is the parent makefile for Linear Buffer Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = infrlb.mak \
	infrlbt.mak

include $(MAKEINC)/make.parent_lib

ifeq ($(OPUP_BLD),yes)
BINMAKEFILES =  lb_cat.mak \
	lb_create.mak \
	lb_info.mak \
	lb_rm.mak \
	lb_rep.mak \
	lb_nt.mak
else
BINMAKEFILES =  lb_cat.mak \
	lb_create.mak \
	lb_info.mak \
	lb_rm.mak \
	lb_rep.mak \
	lb_nt.mak \
	lb_locktst.mak
endif

include $(MAKEINC)/make.parent_bin

