# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2004/10/19 13:48:43 $
# $Id: cpc001_lib003.make,v 1.6 2004/10/19 13:48:43 steves Exp $
# $Revision: 1.6 $
# $State: Exp $

# Makefile for hci library

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = libhci.mak

include $(MAKEINC)/make.parent_lib

