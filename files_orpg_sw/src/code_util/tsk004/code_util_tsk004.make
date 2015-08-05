
# RCS info
# $Author: cheryls $
# $Locker:  $
# $Date: 2004/03/03 17:56:26 $
# $Id: code_util_tsk004.make,v 1.2 2004/03/03 17:56:26 cheryls Exp $
# $Revision: 1.2 $
# $State: Exp $

# This is the parent makefile for CODEview Text (CVT)

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# For CODE. The following target to permit compiling CVT on ORPG Build 4.
all::
	@if [ ! -f $(MAKETOP)/include/orpg_product.h ]; \
	then  cp cvt_orpg_product.h $(MAKETOP)/include/orpg_product.h; fi


BINMAKEFILES = cvt.mak 

include $(MAKEINC)/make.parent_bin

