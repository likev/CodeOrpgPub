# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 1998/05/29 13:23:49 $
# $Id: cpc102_tsk026.make,v 1.2 1998/05/29 13:23:49 aamir Exp $
# $Revision: 1.2 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = pup_emu.mak 

include $(MAKEINC)/make.parent_bin

