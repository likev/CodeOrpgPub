# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/03/03 18:12:07 $
# $Id: cpc013_tsk012.make,v 1.2 2009/03/03 18:12:07 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = dp_dua_accum.mak

include $(MAKEINC)/make.parent_bin
