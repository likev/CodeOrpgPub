# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/07/22 17:11:24 $
# $Id: cpc108_tsk025.make,v 1.1 2013/07/22 17:11:24 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: VCP Sequence initialization

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_vcp_seq.mak 
include $(MAKEINC)/make.parent_bin
