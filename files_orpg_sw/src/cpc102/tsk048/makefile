# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/02/04 14:53:23 $
# $Id: cpc102_tsk048.make,v 1.2 2005/02/04 14:53:23 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

# This is the parent makefile for task_stats tool.

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) diskstat $(TOOLSCRIPTDIR)/diskstat

