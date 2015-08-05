# RCS info
# $Author: cmn $
# $Locker:  $
# $Date: 2011/05/22 16:48:19 $
# $Id: perf_mon_plotter.mak,v 1.1 2011/05/22 16:48:19 cmn Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) perf_mon_plotter.script $(TOOLSCRIPTDIR)/perf_mon_plotter

