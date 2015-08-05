# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2004/02/10 19:40:08 $
# $Id: cpc101_lib003.make,v 1.17 2004/02/10 19:40:08 aamirn Exp $
# $Revision: 1.17 $
# $State: Exp $

# This is the parent makefile for the ORPG Libraries

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

LIBMAKEFILES = liborpg.mak

include $(MAKEINC)/make.parent_lib

