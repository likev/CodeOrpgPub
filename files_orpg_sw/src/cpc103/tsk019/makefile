
# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2012/09/26 14:04:07 $
# $Id: cpc103_tsk019.make,v 1.8 2012/09/26 14:04:07 jing Exp $
# $Revision: 1.8 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(SCRIPTDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(SCRIPTDIR)); fi
	$(INSTALL) $(INSTBINFLAGS) config_device.exp $(SCRIPTDIR)/config_device
	$(INSTALL) $(INSTBINFLAGS) config_procedures.exp $(SCRIPTDIR)/config_procedures.exp
	$(INSTALL) $(INSTBINFLAGS) config_cisco.exp $(SCRIPTDIR)/config_cisco.exp
	$(INSTALL) $(INSTBINFLAGS) config_others.exp $(SCRIPTDIR)/config_others.exp

BINMAKEFILES = config_all.mak 

include $(MAKEINC)/make.parent_bin



