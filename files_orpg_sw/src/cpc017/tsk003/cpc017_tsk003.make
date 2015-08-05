# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2003/07/07 13:35:15 $
# $Id: cpc017_tsk003.make,v 1.2 2003/07/07 13:35:15 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mda1d.mak 

include $(MAKEINC)/make.parent_bin

