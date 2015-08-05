# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2009/03/03 20:18:01 $
# $Id: cpc102_tsk032.make,v 1.2 2009/03/03 20:18:01 steves Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES =	mon_cpu_use.mak mon_mem_use.mak

include $(MAKEINC)/make.parent_bin

