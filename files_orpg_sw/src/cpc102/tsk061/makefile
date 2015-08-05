# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2008/03/28 13:52:57 $
# $Id: cpc102_tsk061.make,v 1.1 2008/03/28 13:52:57 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(TOOLSDATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSDATADIR)); fi
	$(INSTALL) $(INSTDATFLAGS) awips_bias.msg $(TOOLSDATADIR)/awips_bias.msg

BINMAKEFILES = edit_bias.mak

include $(MAKEINC)/make.parent_bin

