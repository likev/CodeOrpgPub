# $Author: priegni $
# $Locker:  $
# $Date: 1998/06/16 16:53:48 $
# $Id: cpc001_tsk012.make,v 1.1 1998/06/16 16:53:48 priegni Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_pstat.mak 

include $(MAKEINC)/make.parent_bin
