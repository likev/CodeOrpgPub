# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2000/08/29 15:21:56 $
# $Id: cpc019_tsk001.make,v 1.1 2000/08/29 15:21:56 steves Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mngred.mak

include $(MAKEINC)/make.parent_bin

