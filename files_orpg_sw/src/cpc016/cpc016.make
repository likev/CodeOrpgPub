# RCS info
# $Author: garyg $
# $Locker:  $
# $Date: 2002/12/16 22:09:36 $
# $Id: cpc016.make,v 1.3 2002/12/16 22:09:36 garyg Exp $
# $Revision: 1.3 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = tsk002 \
      tsk003 \
      tsk004 \
      tsk005 \

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

