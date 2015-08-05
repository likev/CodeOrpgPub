# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2007/08/03 18:47:23 $
# $Id: cpc102_tsk002.make,v 1.2 2007/08/03 18:47:23 jing Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	@if [ -d $(CFGDIR)/bin ]; then set +x; \
	else (set -x; $(MKDIR) $(CFGDIR)/bin); fi
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) f2lb $(TOOLSCRIPTDIR)/f2lb
	$(INSTALL) $(INSTBINFLAGS) lb2f $(TOOLSCRIPTDIR)/lb2f

BINMAKEFILES =

include $(MAKEINC)/make.parent_bin

