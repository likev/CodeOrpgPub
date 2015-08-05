# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2003/07/03 20:46:03 $
# $Id: cpc017_tsk010.make,v 1.1 2003/07/03 20:46:03 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mda3d.mak 

include $(MAKEINC)/make.parent_bin

