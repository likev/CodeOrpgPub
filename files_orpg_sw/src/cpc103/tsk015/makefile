# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/05/25 18:50:09 $
# $Id: cpc103_tsk015.make,v 1.28 2005/05/25 18:50:09 ccalvert Exp $
# $Revision: 1.28 $
# $State: Exp $

# This is the parent make description file for New RPG Offline Tools

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) mscf9600 $(SCRIPTDIR)/mscf9600
