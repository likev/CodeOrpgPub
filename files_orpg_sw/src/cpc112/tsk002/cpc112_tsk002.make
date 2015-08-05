# $Date: 2011/09/19 15:02:48 $
# $Id: cpc112_tsk002.make,v 1.1 2011/09/19 15:02:48 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = file_to_ldm.mak

include $(MAKEINC)/make.parent_bin

