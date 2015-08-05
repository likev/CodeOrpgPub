
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2007/03/23 14:29:39 $
# $Id: cpc103_tsk022.make,v 1.2 2007/03/23 14:29:39 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) audit_logs_manager.script $(SCRIPTDIR)/audit_logs_manager

BINMAKEFILES = audit_logs_gui.mak

include $(MAKEINC)/make.parent_bin

