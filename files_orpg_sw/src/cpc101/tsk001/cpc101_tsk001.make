# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2006/03/28 22:59:36 $
# $Id: cpc101_tsk001.make,v 1.1 2006/03/28 22:59:36 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =  update_alg_data.mak   

include $(MAKEINC)/make.parent_bin

