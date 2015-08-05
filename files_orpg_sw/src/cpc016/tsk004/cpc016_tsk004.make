# RCS info
# $Author: port $
# $Locker:  $
# $Date: 1999/04/20 20:48:57 $
# $Id: cpc016_tsk004.make,v 1.3 1999/04/20 20:48:57 port Exp $
# $Revision: 1.3 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hailprod.mak 

include $(MAKEINC)/make.parent_bin

