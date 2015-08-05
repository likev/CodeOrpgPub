# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 1998/05/07 16:39:05 $
# $Id: cpc015.make,v 1.1 1998/05/07 16:39:05 aamir Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = tsk003 \
      tsk005 \
      tsk007 \
      tsk009 \

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

