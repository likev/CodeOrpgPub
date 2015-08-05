# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2006/01/19 21:33:16 $
# $Id: cpc001_tsk027.make,v 1.2 2006/01/19 21:33:16 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

# This is the parent makefile for HCI RDA Control

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	hci_rdc_legacy.mak hci_rdc_orda.mak

include $(MAKEINC)/make.parent_bin

