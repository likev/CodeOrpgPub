# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2003/07/07 13:41:58 $
# $Id: cpc017_tsk007.make,v 1.1 2003/07/07 13:41:58 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = tda1d.mak 

include $(MAKEINC)/make.parent_bin

