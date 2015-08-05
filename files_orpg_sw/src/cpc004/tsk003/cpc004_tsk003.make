# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 1998/05/06 11:22:40 $
# $Id: cpc004_tsk003.make,v 1.1 1998/05/06 11:22:40 aamir Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = control_rda.mak 

include $(MAKEINC)/make.parent_bin

