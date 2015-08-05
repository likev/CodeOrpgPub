# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/05/13 19:16:52 $
# $Id: cpc105.make,v 1.14 2011/05/13 19:16:52 ccalvert Exp $
# $Revision: 1.14 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# tsk002 - UconX Communications Manager
#		Removed build of uconx comm manager 10/28/98
#		Reintroduced build of uconx comm manager 1/31/2000
# tsk004 - common files (no build required)
# tsk005 - TCP Communications Manager
# tsk006 - Communications Manager "ping"

ifeq ($(OPUP_BLD),yes)
SUBDIRS = tsk005
else
SUBDIRS = tsk002 \
	tsk005 \
	tsk006 
endif

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

