
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2006/06/26 19:43:43 $
# $Id: cpc103_tsk020.make,v 1.1 2006/06/26 19:43:43 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) install_epss.sh $(SCRIPTDIR)/install_epss

BINMAKEFILES = epss_gui.mak

include $(MAKEINC)/make.parent_bin

