# RCS info
# $Author: eddief $
# $Locker:  $
# $Date: 2002/05/14 17:46:08 $
# $Id: cpc100_lib019.make,v 1.1 2002/05/14 17:46:08 eddief Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for smipp Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = libsmia.mak

include $(MAKEINC)/make.parent_lib

BINMAKEFILES = smipp.mak

include $(MAKEINC)/make.parent_bin

