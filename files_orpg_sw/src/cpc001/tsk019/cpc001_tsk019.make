# $Author: priegni $
# $Locker:  $
# $Date: 1999/07/07 19:24:28 $
# $Id: cpc001_tsk019.make,v 1.1 1999/07/07 19:24:28 priegni Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_spp.mak 

include $(MAKEINC)/make.parent_bin
