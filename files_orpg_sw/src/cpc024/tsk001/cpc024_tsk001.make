# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2012/10/15 20:12:24 $
# $Id: cpc024_tsk001.make,v 1.6 2012/10/15 20:12:24 steves Exp $
# $Revision: 1.6 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

#install::
#	@if [ -d $(TOOLSCFGEXTDIR) ]; then set +x; \
#	else (set -x; $(MKDIR) $(TOOLSCFGEXTDIR)); fi
#	$(INSTALL) $(INSTDATFLAGS) product_attr_table.dualpol8bit_test $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) task_attr_table.dualpol8bit_test $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) task_tables.dualpol8bit_test $(TOOLSCFGEXTDIR)

BINMAKEFILES = dualpol8bit.mak dualpol8bit_test.mak

include $(MAKEINC)/make.parent_bin

