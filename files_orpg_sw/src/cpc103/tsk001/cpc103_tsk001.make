
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2008/03/21 21:31:28 $
# $Id: cpc103_tsk001.make,v 1.46 2008/03/21 21:31:28 ccalvert Exp $
# $Revision: 1.46 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) RPG_update_software.script $(SCRIPTDIR)/RPG_update_software

BINMAKEFILES = RPG_update_software_gui.mak

include $(MAKEINC)/make.parent_bin

