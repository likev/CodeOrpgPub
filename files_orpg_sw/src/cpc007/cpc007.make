# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/07/16 14:45:59 $
# $Id: cpc007.make,v 1.13 2014/07/16 14:45:59 steves Exp $
# $Revision: 1.13 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = tsk001 \
	tsk002 \
	tsk003 \
	tsk004 \
	tsk006 \
	tsk008 \
	tsk009 \
	tsk011 \
	tsk012 \
	tsk013 \
	tsk014 \
	tsk015 

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

