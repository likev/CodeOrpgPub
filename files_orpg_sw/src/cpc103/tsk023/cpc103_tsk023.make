
# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/10/03 17:52:24 $
# $Id: cpc103_tsk023.make,v 1.2 2014/10/03 17:52:24 steves Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) site_CD_update.script $(SCRIPTDIR)/site_CD_update
	$(INSTALL) $(INSTBINFLAGS) site_CD_hwaddr.script $(SCRIPTDIR)/site_CD_hwaddr

