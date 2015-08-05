# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2000/01/26 19:42:05 $
# $Id: cpc111_tsk003.make,v 1.1 2000/01/26 19:42:05 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = rpg_ps.mak 

include $(MAKEINC)/make.parent_bin

