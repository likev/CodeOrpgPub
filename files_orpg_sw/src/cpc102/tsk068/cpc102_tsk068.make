# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2012/05/31 21:40:40 $
# $Id: cpc102_tsk068.make,v 1.5 2012/05/31 21:40:40 steves Exp $
# $Revision: 1.5 $
# $State: Exp $

# This is the parent makefile for mon_wb tool

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install:: 
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	$(INSTALL) $(INSTDATFLAGS) mon_wb_gui.xml $(TOOLSCFGDIR)/mon_wb_gui.xml

BINMAKEFILES = mon_wb.mak mon_wb_gui.mak mon_wideband.mak

include $(MAKEINC)/make.parent_bin

