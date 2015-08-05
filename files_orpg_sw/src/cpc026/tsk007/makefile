
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/22 16:14:57 $
# $Id: cpc026_tsk007.make,v 1.1 2005/06/22 16:14:57 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) merge_adapt $(SCRIPTDIR)/merge_adapt

