# RCS info
# $Author: priegni $
# $Locker:  $
# $Date: 2000/06/09 20:56:37 $
# $Id: cpc001_tsk030.make,v 1.1 2000/06/09 20:56:37 priegni Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for HCI Load Shed Categories

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	hci_load.mak

include $(MAKEINC)/make.parent_bin

