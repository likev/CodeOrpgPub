# RCS info
# $Author: aamirn $
# $Locker:  $
# $Date: 2003/05/29 16:54:07 $
# $Id: cpc102_tsk065.make,v 1.3 2003/05/29 16:54:07 aamirn Exp $
# $Revision: 1.3 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = prod_extract.mak 

include $(MAKEINC)/make.parent_bin

