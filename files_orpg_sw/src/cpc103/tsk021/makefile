
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2008/12/15 15:58:04 $
# $Id: cpc103_tsk021.make,v 1.1 2008/12/15 15:58:04 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) update_virus_definition_files.sh $(SCRIPTDIR)/update_virus_definition_files

