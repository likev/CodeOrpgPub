# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2007/05/22 13:43:47 $
# $Id: cpc004_tsk004.make,v 1.5 2007/05/22 13:43:47 steves Exp $
# $Revision: 1.5 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = clutprod_c.mak 

include $(MAKEINC)/make.parent_bin

