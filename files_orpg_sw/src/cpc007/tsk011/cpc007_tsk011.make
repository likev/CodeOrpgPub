# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 1999/07/30 17:15:13 $
# $Id: cpc007_tsk011.make,v 1.1 1999/07/30 17:15:13 steves Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = cmprfape.mak 

include $(MAKEINC)/make.parent_bin

