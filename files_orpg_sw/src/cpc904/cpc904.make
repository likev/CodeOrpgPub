# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2006/05/09 21:33:33 $
# $Id: cpc904.make,v 1.4 2006/05/09 21:33:33 jing Exp $
# $Revision: 1.4 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

ifeq ($(ARCH),lnux_x86)
SUBDIRS =	lib001	\
		tsk003
else
SUBDIRS =       lib001  \
                tsk002  \
                tsk003
endif

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

