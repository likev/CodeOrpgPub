# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2012/10/12 20:48:32 $
# $Id: cpc102_tsk018.make,v 1.3 2012/10/12 20:48:32 steves Exp $
# $Revision: 1.3 $
# $State: Exp $


include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# Prevent configuration files from being copied to tools directory.
#install::
#	@if [ -d $(TOOLSCFGEXTDIR) ]; then set +x; \
#	else (set -x; $(MKDIR) $(TOOLSCFGEXTDIR)); fi
#	$(INSTALL) $(INSTDATFLAGS) product_attr_table.test_base_prods_8bit_combbase $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) product_attr_table.test_base_prods_8bit_refldata $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) task_attr_table.test_base_prods_8bit_combbase $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) task_attr_table.test_base_prods_8bit_refldata $(TOOLSCFGEXTDIR)
#	$(INSTALL) $(INSTDATFLAGS) task_tables.test_base_prods_8bit $(TOOLSCFGEXTDIR)
#
BINMAKEFILES = test_base_prods_8bit.mak 

include $(MAKEINC)/make.parent_bin

