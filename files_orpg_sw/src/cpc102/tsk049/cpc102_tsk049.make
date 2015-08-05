# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2004/10/28 22:41:15 $
# $Id: cpc102_tsk049.make,v 1.2 2004/10/28 22:41:15 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

# This is the parent makefile for HCI RDA Control

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	rdasim_gui.mak

include $(MAKEINC)/make.parent_bin

