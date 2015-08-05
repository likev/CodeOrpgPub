
# RCS info
# $Author: cm $
# $Locker:  $
# $Date: 2013-09-11 13:45:20-05 $
# $Id: orda.make,v 1.10 2013-09-11 13:45:20-05 cm Exp $
# $Revision: 1.10 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(ORDADATADIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(ORDADATADIR)); fi
	@if [ -d $(ORDADATADIR)/bin ]; then set +x; \
	else (set -x; $(MKDIR) $(ORDADATADIR)/bin); fi
	@if [ -d $(ORDADATADIR)/log ]; then set +x; \
	else (set -x; $(MKDIR) $(ORDADATADIR)/log); fi
	@if [ -d $(ORDADATADIR)/scripts ]; then set +x; \
	else (set -x; $(MKDIR) $(ORDADATADIR)/scripts); fi
	$(INSTALL) $(INSTBINFLAGS) bin/rda_hci.jar $(ORDADATADIR)/bin/rda_hci.jar   
	$(INSTALL) $(INSTBINFLAGS) bin/rpg_pmd.jar $(ORDADATADIR)/bin/rpg_pmd.jar   
	$(INSTALL) $(INSTBINFLAGS) scripts/startCh1MSCF.sh $(ORDADATADIR)/scripts/startCh1MSCF.sh
	$(INSTALL) $(INSTBINFLAGS) scripts/startCh2MSCF.sh $(ORDADATADIR)/scripts/startCh2MSCF.sh
	$(INSTALL) $(INSTBINFLAGS) scripts/startCh1RpgPmd.sh $(ORDADATADIR)/scripts/startCh1RpgPmd.sh
	$(INSTALL) $(INSTBINFLAGS) scripts/startCh2RpgPmd.sh $(ORDADATADIR)/scripts/startCh2RpgPmd.sh
