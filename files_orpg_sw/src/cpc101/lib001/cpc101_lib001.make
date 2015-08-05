# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 1999/10/18 16:01:55 $
# $Id: cpc101_lib001.make,v 1.10 1999/10/18 16:01:55 steves Exp $
# $Revision: 1.10 $
# $State: Exp $

# This is the parent makefile for the Legacy RPG Library

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = librpg.mak
include $(MAKEINC)/make.parent_lib

