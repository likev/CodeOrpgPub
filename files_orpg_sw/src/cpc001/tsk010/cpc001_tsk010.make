# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/12/02 23:19:30 $
# $Id: cpc001_tsk010.make,v 1.5 2005/12/02 23:19:30 ccalvert Exp $
# $Revision: 1.5 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_cbm_legacy.mak hci_cbm_orda.mak

include $(MAKEINC)/make.parent_bin








