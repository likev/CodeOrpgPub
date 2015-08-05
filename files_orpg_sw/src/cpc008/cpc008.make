# RCS info
# $Author: john $
# $Locker:  $
# $Date: 1998/06/03 19:53:05 $
# $Id: cpc008.make,v 1.2 1998/06/03 19:53:05 john Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = tsk001 \
      tsk002 \
      tsk004 \
      tsk003 \

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

