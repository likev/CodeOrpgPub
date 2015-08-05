# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2014/02/27 19:23:16 $
# $Id: cpc102_tsk023.make,v 1.6 2014/02/27 19:23:16 ccalvert Exp $
# $Revision: 1.6 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

libinstall::
	rm -f ./orpgumc_rda.c
	cp $(MAKETOP)/src/cpc101/lib003/orpgumc_rda.c ./orpgumc_rda.c
	sed -i 's/RDA_config_value = ORPGRDA_get_rda_config( NULL )/RDA_config_value = ORPGRDA_ORDA_CONFIG/' ./orpgumc_rda.c > /dev/null
	sed -i 's/^ *gbhd->azimuth .*/\/\*&\*\//' ./orpgumc_rda.c > /dev/null
	sed -i 's/^\t*d_hd->azimuth.*/\/\*&\*\//' ./orpgumc_rda.c > /dev/null
	sed -i 's/^ *gbhd->elevation .*/\/\*&\*\//' ./orpgumc_rda.c > /dev/null
	sed -i 's/^\t*d_hd->elevation.*/\/\*&\*\//' ./orpgumc_rda.c > /dev/null

BINMAKEFILES = parse_ldm_file.mak validate_ldm_file.mak testlibl2.mak
LIBMAKEFILES = libl2.mak libl2_static.mak

include $(MAKEINC)/make.parent_bin
include $(MAKEINC)/make.parent_lib

