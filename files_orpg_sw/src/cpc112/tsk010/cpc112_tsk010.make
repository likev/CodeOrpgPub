# $Date: 2011/09/19 15:11:32 $
# $Id: cpc112_tsk010.make,v 1.1 2011/09/19 15:11:32 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = RPG_console_msg_to_ldm.mak

include $(MAKEINC)/make.parent_bin

