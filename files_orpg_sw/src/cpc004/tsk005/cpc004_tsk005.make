
# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/12/03 21:21:08 $
# $Id: cpc004_tsk005.make,v 1.1 2014/12/03 21:21:08 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = wideband_agent.mak

include $(MAKEINC)/make.parent_bin

