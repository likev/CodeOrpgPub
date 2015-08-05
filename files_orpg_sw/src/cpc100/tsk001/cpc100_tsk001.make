# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1998/02/21 21:47:38 $
# $Id: cpc100_tsk001.make,v 1.3 1998/02/21 21:47:38 dodson Exp $
# $Revision: 1.3 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = bcast.mak \
	brecv.mak

include $(MAKEINC)/make.parent_bin

