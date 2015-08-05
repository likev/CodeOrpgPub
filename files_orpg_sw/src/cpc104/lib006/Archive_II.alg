# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/11/07 21:18:09 $
# $Id: Archive_II.alg,v 1.8 2014/11/07 21:18:09 steves Exp $
# $Revision: 1.8 $
# $State: Exp $

    alg_name 		value = Archive_II

    version		value = ;
			name = Version of Level-II data to transmit via LDM;
			type = int;
			range = { 4, 6, 7 };
			permission = [ROC, URC];
			default = Version7_sites:	7,	\
				  Other_sites:		6;
			description = Controls the version of level-II data transmitted via LDM. A change of version takes effect at the start of the next volume scan. Version 6 is full resolution data. Version 7 recombines the data to a more coarse resolution. This version is for reduced network communication bandwidths. Version 4 removes Dual-Pol moments and recombines the data to a more coarse resolution. This version is for severely reduced network communication bandwidths.;

    @site_names.Version7_sites:		PHKI PHKM PHMO PHWA FOP1

