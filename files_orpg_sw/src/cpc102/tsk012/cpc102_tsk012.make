# RCS info
# $Author: cheryls $
# $Locker:  $
# $Date: 2001/07/06 22:16:57 $
# $Id: cpc102_tsk012.make,v 1.2 2001/07/06 22:16:57 cheryls Exp $
# $Revision: 1.2 $
# $State: Exp $

# This is the parent makefile for xpdt display tool
include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = xpdt.mak  

include $(MAKEINC)/make.parent_bin

