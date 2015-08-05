# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2006/06/19 19:14:19 $
# $Id: cpc102_tsk008.make,v 1.3 2006/06/19 19:14:19 jing Exp $
# $Revision: 1.3 $
# $State: Exp $

# This is the parent makefile for task_stats tool.

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
	$(INSTALL) $(INSTBINFLAGS) start_wbs $(TOOLSCRIPTDIR)/start_wbs
	$(INSTALL) $(INSTDATFLAGS) wbs_comms.conf $(TOOLSCFGDIR)/wbs_comms.conf
	$(INSTALL) $(INSTDATFLAGS) wbs_tcp.conf $(TOOLSCFGDIR)/wbs_tcp.conf

BINMAKEFILES = wb_simulator.mak

include $(MAKEINC)/make.parent_bin

