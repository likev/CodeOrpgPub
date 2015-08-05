
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/06/02 14:54:56 $
# $Id: cpc102_tsk001.make,v 1.1 2005/06/02 14:54:56 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the parent make description file for cpc_grep tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) cpc_grep.script $(TOOLSCRIPTDIR)/cpc_grep
