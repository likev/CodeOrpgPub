# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2013/07/22 17:15:11 $
# $Id: cpc108.make,v 1.42 2013/07/22 17:15:11 steves Exp $
# $Revision: 1.42 $
# $State: Exp $

include $(MAKEINC)/make.common
include $(MAKEINC)/make.$(ARCH)

# RPG Maintenance Tasks (RPGMNTTSK)

# tsk001 Initialize Alerting Thresholds/Alert Request Messages
# tsk002 Initialize Product Generation Tables
# tsk003 Initialize RDA Alarms Table
# tsk004 Initialize VCP Data
# tsk005 Initialize Task Information 
# tsk006 Hydrometeorological (Hydromet) Tasks and Datastores
# tsk008 RPG Inter-Task Communication files (ITCs)
# tsk009 Manage RPG Tasks and Datastores
# tsk010 Product Generation Tasks and Datastores
# tsk011 Product Distribution Tasks and Datastores
# tsk012 Load Shed Tasks and Datastores
# tsk013 Initialize Product Attribute Table
# tsk015 le_pipe
# tsk016 Product Database
# tsk017 General Status Message (GSM)
# tsk018 RDA Alarms 
# tsk019 process adaptation data 
# tsk020 Snow Accumulation Algorithm Datastores
# tsk022 initialize the RDA Adaptation Data message
# tsk024 DP QPE Datastores
# tsk025 VCP Sequence Initialization
# tsk099 MLOS and Clutter Datastores

SUBDIRS =	tsk001 \
		tsk002 \
		tsk003 \
		tsk004 \
		tsk005 \
		tsk006 \
		tsk008 \
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
		tsk019 \
		tsk020 \
		tsk022 \
		tsk024 \
		tsk025 \
		tsk099

CURRENT_DIR = .
include $(MAKEINC)/make.subdirs

