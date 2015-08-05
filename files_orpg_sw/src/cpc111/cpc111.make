# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2006/06/12 17:11:50 $
# $Id: cpc111.make,v 1.4 2006/06/12 17:11:50 jing Exp $
# $Revision: 1.4 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = tsk001 \
      		tsk002 \
		tsk003 \
		tsk004 \
		tsk005 \
		tsk006

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

