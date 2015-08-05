# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1999/08/10 20:49:26 $
# $Id: cpc108_tsk011.make,v 1.1 1999/08/10 20:49:26 dodson Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Product Distribution Tasks and Datastores

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_prod_dist.mak
include $(MAKEINC)/make.parent_bin
