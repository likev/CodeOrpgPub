# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/08/15 19:40:52 $
# $Id: cpc102_tsk083.make,v 1.1 2014/08/15 19:40:52 steves Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)


# Copy over library source for deserialization ....
all::
	rm -f ./dpp_filters.c dpp_format.c dpprep.h end_of_elev_proc.c findBragg.h calc_system_PhiDP.c
	cp $(MAKETOP)/src/cpc004/tsk011/dpprep.h .
	cp $(MAKETOP)/src/cpc004/tsk011/findBragg.h .
	cp $(MAKETOP)/src/cpc004/tsk011/calc_system_PhiDP.c .
	cp $(MAKETOP)/src/cpc004/tsk011/dpp_filters.c .
	cp $(MAKETOP)/src/cpc004/tsk011/dpp_format.c .
	cp $(MAKETOP)/src/cpc004/tsk011/end_of_elev_proc.c .

BINMAKEFILES = dpprep_zdr.mak

include $(MAKEINC)/make.parent_bin

