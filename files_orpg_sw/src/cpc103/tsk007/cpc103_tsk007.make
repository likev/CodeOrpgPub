
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/22 17:02:52 $
# $Id: cpc103_tsk007.make,v 1.1 2005/06/22 17:02:52 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) print_log $(SCRIPTDIR)/print_log

