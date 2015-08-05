
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/22 16:58:35 $
# $Id: cpc103_tsk005.make,v 1.1 2005/06/22 16:58:35 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) start_rssd $(SCRIPTDIR)/start_rssd

