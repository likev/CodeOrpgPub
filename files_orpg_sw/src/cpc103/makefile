# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/05/24 20:25:28 $
# $Id: cpc103.make,v 1.30 2011/05/24 20:25:28 ccalvert Exp $
# $Revision: 1.30 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# tsk002 (install_tbd) isn't installed, only extracted, so don't compile it

ifeq ($(OPUP_BLD),yes)
SUBDIRS = tsk003
else
SUBDIRS = tsk001 \
	tsk003 \
	tsk004 \
	tsk005 \
	tsk006 \
	tsk007 \
	tsk008 \
	tsk009 \
	tsk010 \
	tsk012 \
	tsk014 \
	tsk015 \
	tsk016 \
	tsk017 \
	tsk018 \
	tsk019 \
	tsk020 \
	tsk021 \
	tsk022 \
	tsk023 \
	tsk024 \
	tsk026 \
	tsk027
endif

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

