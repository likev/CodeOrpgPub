# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2005/01/07 21:27:39 $
# $Id: cpc006_tsk001.make,v 1.3 2005/01/07 21:27:39 steves Exp $
# $Revision: 1.3 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	status_prod.mak 
include $(MAKEINC)/make.parent_bin

