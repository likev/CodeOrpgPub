
# RCS info
# $Author: garyg $
# $Locker:  $
# $Date: 2006/04/24 20:40:18 $
# $Id: cpc026_tsk010.make,v 1.2 2006/04/24 20:40:18 garyg Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) build_adapt_master $(TOOLSCRIPTDIR)/build_adapt_master
	$(INSTALL) $(INSTBINFLAGS) configure_fti_comms $(TOOLSCRIPTDIR)/configure_fti_comms

