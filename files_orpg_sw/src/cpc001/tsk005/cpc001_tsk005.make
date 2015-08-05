# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 1998/05/04 15:39:54 $
# $Id: cpc001_tsk005.make,v 1.1 1998/05/04 15:39:54 aamir Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_wind.mak 

include $(MAKEINC)/make.parent_bin



