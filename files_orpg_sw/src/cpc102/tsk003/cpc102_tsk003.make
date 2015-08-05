# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/02 14:55:09 $
# $Id: cpc102_tsk003.make,v 1.1 2005/06/02 14:55:09 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent make description file for change_radar tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) change_radar.script $(TOOLSCRIPTDIR)/change_radar
