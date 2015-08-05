# $Author: ccalvert $
# $Locker:  $
# $Date: 2005/08/10 14:00:34 $
# $Id: cpc001_tsk038.make,v 1.2 2005/08/10 14:00:34 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_save_adapt.mak 

include $(MAKEINC)/make.parent_bin
