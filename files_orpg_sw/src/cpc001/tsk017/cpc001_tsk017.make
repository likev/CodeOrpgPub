# $Author: priegni $
# $Locker:  $
# $Date: 1999/06/24 13:50:37 $
# $Id: cpc001_tsk017.make,v 1.1 1999/06/24 13:50:37 priegni Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_nb.mak 

include $(MAKEINC)/make.parent_bin
