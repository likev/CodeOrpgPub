# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 1998/05/05 15:12:00 $
# $Id: cpc002_tsk002.make,v 1.1 1998/05/05 15:12:00 aamir Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = ps_routine.mak 

include $(MAKEINC)/make.parent_bin

