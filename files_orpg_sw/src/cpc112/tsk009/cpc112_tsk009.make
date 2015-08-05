# $Date: 2011/09/19 15:10:06 $
# $Id: cpc112_tsk009.make,v 1.1 2011/09/19 15:10:06 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = RPG_RDA_status_msg_to_ldm.mak RPG_vol_status_msg_to_ldm.mak RPG_wx_status_msg_to_ldm.mak

include $(MAKEINC)/make.parent_bin

