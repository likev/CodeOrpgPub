# $Date: 2010/04/12 17:16:05 $
# $Id: cpc112_tsk004.make,v 1.1 2010/04/12 17:16:05 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

BINMAKEFILES = levelII_stats_ICAO_ldmping_encoder.mak

include $(MAKEINC)/make.parent_bin

