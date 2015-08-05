# RCS info
# $Author: dodson $
# $Locker:  $
# $Date: 1999/07/26 18:07:01 $
# $Id: cpc108_tsk006.make,v 1.1 1999/07/26 18:07:01 dodson Exp $
# $Revision: 1.1 $
# $State: Exp $

# Maintenance Task: Hydrometeorological Tasks

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = mnttsk_hydromet.mak
include $(MAKEINC)/make.parent_bin
