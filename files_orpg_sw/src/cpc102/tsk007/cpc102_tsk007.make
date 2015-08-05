# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2003/05/29 16:54:04 $
# $Id: cpc102_tsk007.make,v 1.4 2003/05/29 16:54:04 aamirn Exp $
# $Revision: 1.4 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = ftnpp.mak 

include $(MAKEINC)/make.parent_bin
