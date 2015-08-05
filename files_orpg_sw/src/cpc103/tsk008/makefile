
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/22 17:04:31 $
# $Id: cpc103_tsk008.make,v 1.1 2005/06/22 17:04:31 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) rescue_data $(SCRIPTDIR)/rescue_data

