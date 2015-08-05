# $Date: 2011/09/19 15:17:19 $
# $Id: cpc112_tsk005.make,v 1.2 2011/09/19 15:17:19 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = monitor_archive_II.mak

include $(MAKEINC)/make.parent_bin

