
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/22 17:12:24 $
# $Id: cpc103_tsk012.make,v 1.1 2005/06/22 17:12:24 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) update_rda_gui $(SCRIPTDIR)/update_rda_gui

