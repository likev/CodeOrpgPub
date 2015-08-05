# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2004/01/28 20:40:52 $
# $Id: cpc100_tsk012.make,v 1.1 2004/01/28 20:40:52 jing Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mping.mak

include $(MAKEINC)/make.parent_bin

