# RCS info
# $Author: ryans $
# $Locker:  $
# $Date: 2006/03/08 21:11:44 $
# $Id: cpc102_tsk039.make,v 1.1 2006/03/08 21:11:44 ryans Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for prod_deserialize tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = prod_deserialize.mak

include $(MAKEINC)/make.parent_bin

