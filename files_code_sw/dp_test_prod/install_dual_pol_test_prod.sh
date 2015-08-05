#!/usr/bin/ksh
#
# FILE: install_dual_pol_test_prod.sh
#
#
#############################################################################
#
# This script installs the dual pol test products: 340-344, 600-605, 700-705. 
# It should be in ~/cfg/dp-test directory after un tared.  It must be 
# executed from that directory.
#
#############################################################################

print "\nInstalling dual pol test products \n"
print "Must be executed from the cfg directory\n"

# create directory extensions in ~/cfg
mkdir -p ~/cfg/extensions

# copy files to proper location
print "\n Copy configuration files to ~/cfg/extensions \n"

print "  product_generation_tables.dualpol8bit_test \n"
cp -p ./product_generation_tables.dualpol8bit_test ~/cfg/extensions/.
print "  product_generation_tables.test_base_prods_8bit \n"
cp -p ./product_generation_tables.test_base_prods_8bit ~/cfg/extensions/.

cd ~/src/cpc024/tsk001
print "  product_attr_table.dualpol8bit_test \n"
cp -p ./product_attr_table.dualpol8bit_test ~/cfg/extensions/.
print "  task_attr_table.dualpol8bit_test \n"
cp -p ./task_attr_table.dualpol8bit_test ~/cfg/extensions/.
print "  task_tables.dualpol8bit_test \n"
cp -p ./task_tables.dualpol8bit_test ~/cfg/extensions/.

cd ~/src/cpc102/tsk018
print "  product_attr_table.test_base_prods_8bit_combbase \n"
cp -p ./product_attr_table.test_base_prods_8bit_combbase ~/cfg/extensions/.
print "  product_attr_table.test_base_prods_8bit_refldata \n"
cp -p ./product_attr_table.test_base_prods_8bit_refldata ~/cfg/extensions/.
print "  task_attr_table.test_base_prods_8bit_combbase \n"
cp -p ./task_attr_table.test_base_prods_8bit_combbase ~/cfg/extensions/.
print "  task_attr_table.test_base_prods_8bit_refldata \n"
cp -p ./task_attr_table.test_base_prods_8bit_refldata ~/cfg/extensions/.
print "  task_tables.test_base_prods_8bit \n"
cp -p ./task_tables.test_base_prods_8bit ~/cfg/extensions/.

print "\n Done\n"

