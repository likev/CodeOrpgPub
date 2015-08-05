# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2013/03/14 19:43:30 $
# $Id: cpc102_tsk074.make,v 1.1 2013/03/14 19:43:30 ccalvert Exp $
# $Revision: 1.1 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

install::
	@if [ -d $(TOOLSCFGDIR) ]; then set +x; \
	else (set -x; $(MKDIR) $(TOOLSCFGDIR)); fi
	$(INSTALL) $(INSTDATFLAGS) task_attr_table.save_product_db_lb $(TOOLSCFGDIR)/task_attr_table.save_product_db_lb
	$(INSTALL) $(INSTDATFLAGS) task_tables.save_product_db_lb $(TOOLSCFGDIR)/task_tables.save_product_db_lb

BINMAKEFILES =  save_product_db_lb.mak

include $(MAKEINC)/make.parent_bin

