# $Date: 2011/09/19 15:14:57 $
# $Id: cpc112_tsk014.make,v 1.1 2011/09/19 15:14:57 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = RPG_prod_info_to_ldm.mak

include $(MAKEINC)/make.parent_bin

