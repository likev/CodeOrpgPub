# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/05/17 19:43:52 $
# $Id: cpc100_tsk014.make,v 1.3 2005/05/17 19:43:52 ccalvert Exp $
# $Revision: 1.3 $
# $State: Exp $

# This is the parent makefile for Generic Services Utilities
# (a.k.a. Infrastructure Utilities)
# These are scripts, binaries, etc. that are not directly related to
# any other generic services elements.

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	prm.mak

include $(MAKEINC)/make.parent_bin

