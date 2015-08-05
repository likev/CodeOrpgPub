# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2007/01/26 14:46:28 $
# $Id: cpc102_tsk047.make,v 1.4 2007/01/26 14:46:28 jing Exp $
# $Revision: 1.4 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# Copy over library source for deserialization ....
all::
	rm -f ./standalone_dsp.c
	cp ./display_status_prod.c ./standalone_dsp.c

BINMAKEFILES = display_status_prod.mak standalone_dsp.mak

include $(MAKEINC)/make.parent_bin

