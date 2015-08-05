# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2002/12/16 15:21:52 $
# $Id: cpc004_tsk008.make,v 1.1 2002/12/16 15:21:52 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = elev_prod.mak

include $(MAKEINC)/make.parent_bin

