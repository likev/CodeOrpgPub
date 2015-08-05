
# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/02/19 16:19:44 $
# $Id: cpc103_tsk026.make,v 1.1 2010/02/19 16:19:44 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) security_log.script $(SCRIPTDIR)/security_log

BINMAKEFILES = security_log_extract.mak

include $(MAKEINC)/make.parent_bin

