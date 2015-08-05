# RCS info
# $Author: jclose $
# $Locker:  $
# $Date: 2009/10/06 17:06:15 $
# $Id: cpc102_tsk015.make,v 1.5 2009/10/06 17:06:15 jclose Exp $
# $Revision: 1.5 $
# $State: Exp $

# This is the parent makefile for task_stats tool.

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	$(INSTALL) $(INSTDATFLAGS) validate_a2_b11.xml $(TOOLSCFGDIR)/validate_a2_b11.xml
	$(INSTALL) $(INSTDATFLAGS) validate_a2_b12.xml $(TOOLSCFGDIR)/validate_a2_b12.xml

BINMAKEFILES = validate_a2.mak

include $(MAKEINC)/make.parent_bin

