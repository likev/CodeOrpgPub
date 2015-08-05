# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2004/10/18 21:35:04 $
# $Id: cpc108_tsk004.make,v 1.1 2004/10/18 21:35:04 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: VCP initialization

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_vcp.mak 
include $(MAKEINC)/make.parent_bin
