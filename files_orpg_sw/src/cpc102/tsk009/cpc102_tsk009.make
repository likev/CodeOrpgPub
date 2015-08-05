# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/01/08 15:57:10 $
# $Id: cpc102_tsk009.make,v 1.1 2009/01/08 15:57:10 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_status_print.mak 

include $(MAKEINC)/make.parent_bin
