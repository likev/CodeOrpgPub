# RCS info
# $Author: eddief $
# $Locker:  $
# $Date: 2001/03/13 16:14:42 $
# $Id: cpc001_tsk032.make,v 1.1 2001/03/13 16:14:42 eddief Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	hci_init_config.mak

include $(MAKEINC)/make.parent_bin

