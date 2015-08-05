# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2004/11/11 15:23:35 $
# $Id: cpc005_tsk003.make,v 1.2 2004/11/11 15:23:35 steves Exp $
# $Revision: 1.2 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = prfselect.mak 

include $(MAKEINC)/make.parent_bin


