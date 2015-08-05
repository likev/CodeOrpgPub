# RCS info
# $Author: port $
# $Locker:  $
# $Date: 1999/04/20 18:33:33 $
# $Id: cpc008_tsk003.make,v 1.3 1999/04/20 18:33:33 port Exp $
# $Revision: 1.3 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = combattr.mak 

include $(MAKEINC)/make.parent_bin

