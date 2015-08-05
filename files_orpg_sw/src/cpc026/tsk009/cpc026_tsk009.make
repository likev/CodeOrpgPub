# RCS info
# $Author: ryans $
# $Locker:  $
# $Date: 2005/03/29 16:59:40 $
# $Id: cpc026_tsk009.make,v 1.2 2005/03/29 16:59:40 ryans Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =  merge_adapt_data.mak   

include $(MAKEINC)/make.parent_bin

