# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2015/01/08 16:36:21 $
# $Id: Nonoperational.alg,v 1.14 2015/01/08 16:36:21 steves Exp $
# $Revision: 1.14 $
# $State: Exp $

    alg_name 		value = Wideband Backup Communications

    test_sz2prf		value = No;	
			name = Test SZ2 PRF Selection in Conjunction with SAILS;
			type = string;		
			range = {No, Yes};	
			description = Allows Testing SZ2 PRF Selection with SAILS;
			enum =  0, 1;	

    wb_prim_poll	value = 5;
			name = Wideband Primary Comms Polling Interval;
			type = int;
			range = [5, 120];
			unit = secs;
			description = Number of seconds between SNMP calls to check whether wideband's primary comms is up or down.;

    wb_prim_stbl	value = 60;
			name = Wideband Primary Comms Stability Interval;
			type = int;
			range = [0, 1800];
			unit = secs;
			description = Number of seconds wideband is on primary comms before clearing alarm;

    wb_prim_monitor	value = Yes;
			name = Wideband Primary Comms Monitor Flag;
			type = string;
			range = {No, Yes};
			permission = [URC];
			description = Allows monitoring of wideband primary comms;
			enum =  0, 1;	
