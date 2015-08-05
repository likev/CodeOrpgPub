# RCS info
# $Author: 
# $Locker:  $
# $Date: 1999/06/04 14:29:08 $
# $Id: cpc020.make,v 1.3 1999/06/04 14:29:08 dan Exp $
# $Revision: 1.3 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# This is the parent makefile for CPC020 RMS interface

#   lib001 Port manager
#   lib002 Utilities
#   lib003 Message handling
#   tsk001 RMS interface main program


SUBDIRS =	lib001 \
		lib002 \
		lib003 \
		tsk004

CURRENT_DIR = .

include $(MAKEINC)/make.subdirs
