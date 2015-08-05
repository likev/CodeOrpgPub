# $Date: 2011/09/19 15:15:52 $
# $Id: cpc112_tsk016.make,v 1.1 2011/09/19 15:15:52 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = RPG_save_log_to_ldm.mak

include $(MAKEINC)/make.parent_bin

