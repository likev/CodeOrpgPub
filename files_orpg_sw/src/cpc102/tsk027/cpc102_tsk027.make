# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/12/09 22:28:20 $
# $Id: cpc102_tsk027.make,v 1.3 2014/12/09 22:28:20 steves Exp $
# $Revision: 1.3 $
# $State: Exp $

# This is the parent makefile for Reliable Broadcast Services

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = prod_stat.mak standalone_pstat.mak

include $(MAKEINC)/make.parent_bin

