# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2009/08/19 15:11:39 $
# $Id: code_util_tsk001.make,v 1.25 2009/08/19 15:11:39 ccalvert Exp $
# $Revision: 1.25 $
# $State: Exp $

# This is the parent makefile for CVG

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

## CVG 8.3 added installation of integrated map data files
## CVG 9.0 MOVED installation of integrated map data files to code_util/lib001

install::
	@if [ -d $(MAKETOP)/tools ]; then set +x; \
	else (set -x; $(MKDIR) $(MAKETOP)/tools); fi
	@if [ -d $(MAKETOP)/tools/cvg_map ]; then set +x; \
	else (set -x; $(MKDIR) $(MAKETOP)/tools/cvg_map); fi
	$(INSTALL) $(INSTBINFLAGS) create_all_cvg_maps $(MAKETOP)/tools/cvg_map/create_all_cvg_maps
	$(INSTALL) $(INSTBINFLAGS) create_sample_maps $(MAKETOP)/tools/cvg_map/create_sample_maps

## CVG 8 added cvg_read_db.mak
## CVG 8.3 added map_cvg.mak
## CVG 9.1 added edit_cvgplt.mak and removed cvg_color_edit.mak

BINMAKEFILES = cvg.mak cvg_read_db.mak edit_cvgplt.mak map_cvg.mak

include $(MAKEINC)/make.parent_bin

