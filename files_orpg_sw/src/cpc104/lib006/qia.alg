# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2011/09/27 15:04:38 $
# $Id: qia.alg,v 1.2 2011/09/27 15:04:38 ccalvert Exp $
# $Revision: 1.2 $
# $State: Exp $

###########################################################
##
##	File:		qia.alg
##	Author:		Brian Klein
##	Date:		Jan 18, 2011
##	version:	1.0
##
##	Description:
##	------------
##
##	This file contains all the adaptation data used by 
##	the Quality Index Algorithm.
##
###########################################################

  alg_name		value = QIA

  z_atten_thresh	value = 25.0;
			name = Reflectivity Threshold for Attenuation;
			type = float;
			range = [0.0, 60.0];
			unit = dBZ;
			accuracy = [0.5];
			description = Determines when to eliminate RhoHV from QI computation due to attenuation;
