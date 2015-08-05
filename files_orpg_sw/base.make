# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1998/02/24 16:51:16 $
# $Id: base.make,v 1.1 1998/02/24 16:51:16 dodson Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

SUBDIRS = src

CURRENT_DIR = .

include $(MAKEINC)/make.subdirs
