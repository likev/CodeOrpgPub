
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2006/02/21 16:55:31 $
# $Id: cpc026_tsk002.make,v 1.2 2006/02/21 16:55:31 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) build_adapt_media $(TOOLSCRIPTDIR)/build_adapt_media

