# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 1998/05/04 15:33:48 $
# $Id: cpc001_tsk002.make,v 1.1 1998/05/04 15:33:48 aamir Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_prod.mak 

include $(MAKEINC)/make.parent_bin

