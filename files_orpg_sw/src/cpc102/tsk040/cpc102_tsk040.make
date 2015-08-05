# RCS info
# $Author: ryans $
# $Locker:  $
# $Date: 2007/05/23 21:04:00 $
# $Id: cpc102_tsk040.make,v 1.1 2007/05/23 21:04:00 ryans Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for print_iprod tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = print_iprod.mak

include $(MAKEINC)/make.parent_bin

