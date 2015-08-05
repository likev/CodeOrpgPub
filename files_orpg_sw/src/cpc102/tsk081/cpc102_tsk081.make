# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/05/13 19:17:57 $
# $Id: cpc102_tsk081.make,v 1.1 2014/05/13 19:17:57 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = prod_req.mak

include $(MAKEINC)/make.parent_bin

