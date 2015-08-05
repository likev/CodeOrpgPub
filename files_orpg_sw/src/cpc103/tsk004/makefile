
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/06/04 17:27:46 $
# $Id: cpc103_tsk004.make,v 1.3 2009/06/04 17:27:46 ccalvert Exp $
# $Revision: 1.3 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) save_log.script $(SCRIPTDIR)/save_log

BINMAKEFILES = save_log_gui.mak save_log_extract.mak

include $(MAKEINC)/make.parent_bin

