
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2008/03/21 21:33:57 $
# $Id: cpc103_tsk017.make,v 1.1 2008/03/21 21:33:57 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) ROC_update.script $(SCRIPTDIR)/ROC_update

