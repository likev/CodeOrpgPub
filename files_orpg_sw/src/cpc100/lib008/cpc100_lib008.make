# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2007/02/01 19:38:00 $
# $Id: cpc100_lib008.make,v 1.13 2007/02/01 19:38:00 jing Exp $
# $Revision: 1.13 $
# $State: Exp $

# This is the parent makefile for Remote Tool Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES =	libinfrt.mak \
		libinfr.mak

include $(MAKEINC)/make.parent_lib


