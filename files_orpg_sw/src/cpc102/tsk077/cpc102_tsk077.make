# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2014/07/21 20:11:46 $
# $Id: cpc102_tsk077.make,v 1.3 2014/07/21 20:11:46 ccalvert Exp $
# $Revision: 1.3 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = hci_sails_tool.mak test_orpgsails.mak

include $(MAKEINC)/make.parent_bin

