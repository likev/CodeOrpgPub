# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2000/11/20 16:54:27 $
# $Id: cpc108_tsk002.make,v 1.1 2000/11/20 16:54:27 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Product Generation Tables

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_pgt.mak
include $(MAKEINC)/make.parent_bin
