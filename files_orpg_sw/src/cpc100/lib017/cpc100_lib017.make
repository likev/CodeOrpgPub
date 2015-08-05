# RCS info
# $Author: cm $
# $Locker:  $
# $Date: 1998/12/03 12:46:10 $
# $Id: cpc100_lib017.make,v 1.1 1998/12/03 12:46:10 cm Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent makefile for Linear Buffer Services
LB_SRC_DIR = ../lib003
include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)
include make.lb_common

LIBMAKEFILES = lb_lib.mak

include $(MAKEINC)/make.parent_lib


