# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2008/01/16 21:13:30 $
# $Id: cpc104.make,v 1.6 2008/01/16 21:13:30 jing Exp $
# $Revision: 1.6 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = lib001 \
      lib002 \
      lib003 \
      lib004 \
      lib005 \
      lib006 \
      lib007 \
      lib009 \

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

