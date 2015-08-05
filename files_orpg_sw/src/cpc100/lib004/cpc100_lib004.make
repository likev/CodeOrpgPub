# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2000/08/25 21:41:00 $
# $Id: cpc100_lib004.make,v 1.22 2000/08/25 21:41:00 steves Exp $
# $Revision: 1.22 $
# $State: Exp $

# This is the parent makefile for the Miscellaneous Library

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


LIBMAKEFILES = libmisc.mak
include $(MAKEINC)/make.parent_lib

