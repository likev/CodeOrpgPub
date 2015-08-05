# RCS info
# $Author: jing $
# $Locker:  $
# $Date: 2004/02/26 19:41:51 $
# $Id: cpc102_tsk066.make,v 1.7 2004/02/26 19:41:51 jing Exp $
# $Revision: 1.7 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = play_a2.mak \
		save_volume_file.mak

include $(MAKEINC)/make.parent_bin

