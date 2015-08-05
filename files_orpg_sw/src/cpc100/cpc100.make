# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/07 20:22:55 $
# $Id: cpc100.make,v 1.27 2014/03/07 20:22:55 steves Exp $
# $Revision: 1.27 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# Generic Services Libraries, Daemons, and Binaries

# tsk001 bcast/brecv
# lib002 liben.* Event Notification Services
# lib003 libinfrlb.* Linear Buffer Services for inclusion in libinfr only
# lib004 libmisc.* Miscellaneous Services
# lib005 librmt.* Remote Tool
# lib006 librss.* Remote System Services
# lib007 libsupp.* Supplemental Services
# lib008 libinfr.* (all-encompassing infrastructure library)
# lib009 libMPS.* (UconX)
# lib010 libMALRM.* Multiple Alarm Services
# lib011 libfreeway.* Simpact Freeway Library (COTS) - removed 6/27/01
# tsk012 mping
# tsk014 Utilities (scripts, binaries, etc.)
# lib016 Net Utilities 
# lib017 liblb.* Linear Buffer Services (standalone version) 

# NOTE: lib008 should be last library in list, since it is a compilation
#       of several infrastructure libraries

# NOTE: At this time, we are unable to build the UconX, Simpact, and ISL
#       libraries for lnux_x86
ifeq ($(OPUP_BLD),yes)
SUBDIRS =	lib002 \
		lib003 \
		lib004 \
		lib005 \
		lib006 \
		lib007 \
		lib010 \
		lib016 \
		lib019 \
		lib008

else
ifeq ($(ARCH),lnux_x86)
SUBDIRS =	tsk001 \
		lib002 \
		lib003 \
		lib004 \
		lib005 \
		lib006 \
		lib007 \
		lib010 \
		tsk012 \
		tsk014 \
		lib016 \
		lib017 \
		lib019 \
		lib020 \
		lib008
else
SUBDIRS =	tsk001 \
		lib002 \
		lib003 \
		lib004 \
		lib005 \
		lib006 \
		lib007 \
		lib010 \
		tsk012 \
		tsk014 \
		lib016 \
		lib017 \
		lib019 \
		lib020 \
		lib008
endif
endif

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

