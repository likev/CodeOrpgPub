# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/06/13 21:11:27 $
# $Id: cpc101_lib006.make,v 1.1 2013/06/13 21:11:27 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for the ORPG Libraries

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = liborpgsun.mak

include $(MAKEINC)/make.parent_lib

