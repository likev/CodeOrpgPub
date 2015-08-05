# RCS info
# $Author: eforren $
# $Locker:  $
# $Date: 2000/12/04 22:54:34 $
# $Id: cpc026_tsk005.make,v 1.1 2000/12/04 22:54:34 eforren Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for the Adaptation Data and Product
# Request LB-generation ORPG Tools

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	adapt_data.mak \
		adapt_data_gen.mak 
include $(MAKEINC)/make.parent_bin

