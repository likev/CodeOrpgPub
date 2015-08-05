# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2006/01/03 16:54:01 $
# $Id: cpc007_tsk014.make,v 1.3 2006/01/03 16:54:01 steves Exp $
# $Revision: 1.3 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = bvel8bit.mak 

include $(MAKEINC)/make.parent_bin

