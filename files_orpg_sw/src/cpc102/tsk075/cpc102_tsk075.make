# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/05/30 19:27:25 $
# $Id: cpc102_tsk075.make,v 1.1 2013/05/30 19:27:25 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for the model data viewer tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = modelviewer.mak

include $(MAKEINC)/make.parent_bin



