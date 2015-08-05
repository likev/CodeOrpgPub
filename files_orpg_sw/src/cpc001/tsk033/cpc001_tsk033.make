# $Author: jing $
# $Locker:  $
# $Date: 2003/06/20 16:00:26 $
# $Id: cpc001_tsk033.make,v 1.1 2003/06/20 16:00:26 jing Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_apps_adapt.mak 

include $(MAKEINC)/make.parent_bin
