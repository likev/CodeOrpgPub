# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2008/05/14 16:15:51 $
# $Id: cpc013.make,v 1.7 2008/05/14 16:15:51 ccalvert Exp $
# $Revision: 1.7 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# This is the parent makefile for CPC013 Precipitation Algorithms

#   tsk001 VIL/Echo Tops Algorithm
#   tsk003 Precipitation Preprocessing Algorithm
#   tsk004 
#   tsk005 Precipitation Rate and Accumulation Algorithm
#   tsk006 Precipitation Adjustment Algorithm
#   tsk008 Snow Algorithm

SUBDIRS =	tsk001 \
		tsk003 \
		tsk005 \
		tsk006 \
		tsk008 \
		tsk009 \
		tsk010 \
		tsk011 \
		tsk012

CURRENT_DIR = .

include $(MAKEINC)/make.subdirs

