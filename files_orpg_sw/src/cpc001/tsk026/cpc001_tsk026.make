# RCS info
# $Author: priegni $
# $Locker:  $
# $Date: 2000/05/09 16:43:48 $
# $Id: cpc001_tsk026.make,v 1.1 2000/05/09 16:43:48 priegni Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for HCI RDA/RPG Interface Control/Status

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	hci_rda_link.mak

include $(MAKEINC)/make.parent_bin

