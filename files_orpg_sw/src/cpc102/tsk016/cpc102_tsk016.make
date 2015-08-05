# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/01/08 17:01:57 $
# $Id: cpc102_tsk016.make,v 1.1 2010/01/08 17:01:57 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = change_dea_value.mak 

include $(MAKEINC)/make.parent_bin
