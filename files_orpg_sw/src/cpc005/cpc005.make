# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2004/12/21 15:30:26 $
# $Id: cpc005.make,v 1.6 2004/12/21 15:30:26 steves Exp $
# $Revision: 1.6 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# This is the parent makefile for CPC005 Rain Gage Processing

#   tsk002 Precipitation Detection/Automatic PRF Selection Algorithm
#   tsk003 Automatic PRF Selection Algorithm
#   tsk004 PRF - Range Obscuration Bit Map Algorithm/Product


SUBDIRS =	tsk002 \
		tsk003 \
		tsk004 

CURRENT_DIR = .

include $(MAKEINC)/make.subdirs
