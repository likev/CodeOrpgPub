# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2012/10/09 16:12:53 $
# $Id: cpc112.make,v 1.22 2012/10/09 16:12:53 ccalvert Exp $
# $Revision: 1.22 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# Task Definition
# tsk001 convert_ldm
# tsk002 file_to_ldm
# tsk003 adapt_to_ldm
# tsk004 levelII_stats_ICAO_ldmping_encoder
# tsk005 monitor_archive_II
# tsk006 lb_to_ldm
# tsk007 RPG_status_log_to_ldm, RPG_error_log_to_ldm
# lib008 RPG LDM lib
# tsk009 RPG_RDA_status_msg_to_ldm
# tsk009 RPG_vol_status_msg_to_ldm
# tsk009 RPG_wx_status_msg_to_ldm
# tsk010 RPG_console_msg_to_ldm
# tsk011 RPG_info_to_ldm
# tsk012 RPG_task_status_to_ldm
# tsk013 RPG_RUC_data_to_ldm
# tsk013 RPG_bias_table_to_ldm
# tsk014 RPG_prod_info_to_ldm
# tsk015 RPG_LDM_write_read_monitor
# tsk016 save_log_to_ldm
# tsk017 RPG_hw_stats_to_ldm
# tsk018 RPG_L3_to_ldm
# tsk019 RPG_snmp_to_ldm

SUBDIRS = tsk001 \
	tsk002 \
	tsk003 \
	tsk004 \
	tsk005 \
	tsk006 \
	tsk007 \
	lib008 \
	tsk009 \
	tsk010 \
	tsk011 \
	tsk012 \
	tsk013 \
	tsk014 \
	tsk015 \
	tsk016 \
	tsk017 \
	tsk018 \
	tsk019

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs
