# RCS info
# $Author: nolitam $
# $Locker:  $
# $Date: 2002/11/26 13:55:24 $
# $Id: cpc004_tsk006.make,v 1.2 2002/11/26 13:55:24 nolitam Exp $
# $Revision: 1.2 $
# $State: Exp $
include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = recclalg.mak 

include $(MAKEINC)/make.parent_bin
