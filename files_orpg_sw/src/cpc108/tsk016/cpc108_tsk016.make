# RCS info
# $Author: aamir $
# $Locker:  $
# $Date: 2000/03/01 19:28:20 $
# $Id: cpc108_tsk016.make,v 1.1 2000/03/01 19:28:20 aamir Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Init MLOS and RDA MSGS MNTTSK code

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_prod_database.mak
include $(MAKEINC)/make.parent_bin
