# RCS info
# $Author: port $
# $Locker:  $
# $Date: 1999/04/20 20:37:14 $
# $Id: cpc015_tsk009.make,v 1.4 1999/04/20 20:37:14 port Exp $
# $Revision: 1.4 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hailalg.mak 

include $(MAKEINC)/make.parent_bin

