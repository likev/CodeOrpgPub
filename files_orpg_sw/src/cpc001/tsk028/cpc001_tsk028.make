# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2006/01/19 21:39:15 $
# $Id: cpc001_tsk028.make,v 1.3 2006/01/19 21:39:15 ccalvert Exp $
# $Revision: 1.3 $
# $State: Exp $

# This is the parent makefile for HCI RDA Alarms

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	hci_rda_legacy.mak hci_rda_orda.mak

include $(MAKEINC)/make.parent_bin

