# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2011/03/21 17:04:16 $
# $Id: cpc102_tsk006.make,v 1.2 2011/03/21 17:04:16 steves Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_basedata_tool.mak basedata_tool.mak

include $(MAKEINC)/make.parent_bin
