
# RCS info
# $Author: garyg $
# $Locker:  $
# $Date: 2002/02/12 18:25:15 $
# $Id: cpc102_tsk059.make,v 1.1 2002/02/12 18:25:15 garyg Exp $
# $Revision: 1.1 $
# $State: Exp $

# This is the makefile for the switch_rda script

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	@if [ -d $(TOOLSCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) switch_rda.script $(TOOLSCRIPTDIR)/switch_rda
