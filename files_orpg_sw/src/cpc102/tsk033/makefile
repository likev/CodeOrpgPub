# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2012/10/15 14:41:14 $
# $Id: cpc102_tsk033.make,v 1.3 2012/10/15 14:41:14 steves Exp $
# $Revision: 1.3 $
# $State: Exp $

DEAUDATADIR = dea

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

#install::
#	@if [ -d $(TOOLSCFGEXTDIR) ]; then set +x; \
#	else (set -x; $(MKDIR) $(TOOLSCFGEXTDIR)); fi
#	@if [ -d $(TOOLSCFGDIR)/$(DEAUDATADIR) ]; then set +x; \
#	else (set -x; $(MKDIR) $(TOOLSCFGDIR)/$(DEAUDATADIR)); fi
#	$(INSTALL) $(INSTDATFLAGS) data_attr_table.sensitivity_loss $(TOOLSCFGEXTDIR)/data_attr_table.sensitivity_loss
#	$(INSTALL) $(INSTDATFLAGS) pbd.alg $(TOOLSCFGDIR)/$(DEAUDATADIR)/pbd.alg

BINMAKEFILES = pbd_snr.mak 

include $(MAKEINC)/make.parent_bin

