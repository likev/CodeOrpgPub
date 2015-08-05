# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2003/08/25 14:30:08 $
# $Id: cpc102_tsk062.make,v 1.1 2003/08/25 14:30:08 aamirn Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the makefile for product compare tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =  prod_cmpr.mak  

 
include $(MAKEINC)/make.parent_bin

