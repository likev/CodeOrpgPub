# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 1998/05/06 11:21:22 $
# $Id: cpc004_tsk001.make,v 1.1 1998/05/06 11:21:22 aamir Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = veldeal.mak 

include $(MAKEINC)/make.parent_bin

