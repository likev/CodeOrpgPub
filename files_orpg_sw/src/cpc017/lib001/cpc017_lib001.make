# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2003/07/03 13:28:01 $
# $Id: cpc017_lib001.make,v 1.1 2003/07/03 13:28:01 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for the Legacy AP-Removed Common Modules 
# Library

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = libtda.mak
include $(MAKEINC)/make.parent_lib

