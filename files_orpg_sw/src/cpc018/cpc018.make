# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2009/02/25 16:12:12 $
# $Id: cpc018.make,v 1.17 2009/02/25 16:12:12 steves Exp $
# $Revision: 1.17 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = tsk001 \
      tsk003 \
      tsk005 \
      tsk007 \
      tsk010

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

