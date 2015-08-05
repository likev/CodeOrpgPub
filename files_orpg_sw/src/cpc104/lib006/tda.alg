# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/12 20:57:07 $
# $Id: tda.alg,v 1.7 2010/05/12 20:57:07 ccalvert Exp $
# $Revision: 1.7 $
# $State: Exp $

    alg_name		value = Tornado Detection

    min_dbz_thresh	value = 0;	
			name = Min Reflectivity;	
			type = int;		
			range = [-20, 20];	
			unit = dBZ;		
			permission = [ROC, URC];
			description = The minimum reflectivity value required in a SAMPLE VOLUME for it to be used in a 1D feature.  Larger values avoid false alarms in weak reflectivity regions.;

    vector_vel_diff	value = 11;	
			name = Vector Velocity Difference;	
			type = int;		
			range = [10, 75];	
			unit = m/s;		
			description = The minimum required gate-to-gate velocity difference;	

    max_pv_range	value = 100;	
			name = Max Pattern Vector Range;	
			type = int;		
			range = [0, 230];	
			unit = km;		
			permission = [ROC, URC];
			description = The maximum range at which 1D features are identified.  TVS detections are not made beyond this range.;

    max_pv_height	value = 10.0;	
			name = Max Pattern Vector Height;	
			type = double;		
			range = [0.0, 15.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum height at which pattern vectors are identified.;	

    max_pv_num		value = 2500;	
			name = Max # of Pattern Vectors;	
			type = int;		
			range = [1500, 3000];	
			description = Maximum number of pattern vectors the algorithm can;	

    diff_vel1		value = 11;	
			name = Differential Velocity #1;	
			type = int;		
			range = [10, 75];	
			unit = m/s;		
			description = The 1st of six velocity difference threshold used as;	

    diff_vel2		value = 15;	
			name = Differential Velocity #2;	
			type = int;		
			range = [15, 80];	
			unit = m/s;		
			description = The 2nd of six velocity difference threshold used as;	

    diff_vel3		value = 20;	
			name = Differential Velocity #3;	
			type = int;		
			range = [20, 85];	
			unit = m/s;		
			description = The 3rd of six velocity difference threshold used as;	

    diff_vel4		value = 25;	
			name = Differential Velocity #4;	
			type = int;		
			range = [25, 90];	
			unit = m/s;		
			description = The 4th of six velocity difference threshold used as;	

    diff_vel5		value = 30;	
			name = Differential Velocity #5;	
			type = int;		
			range = [30, 95];	
			unit = m/s;		
			description = The 5th of six velocity difference threshold used as;	

    diff_vel6		value = 35;	
			name = Differential Velocity #6;	
			type = int;		
			range = [35, 100];	
			unit = m/s;		
			description = The 6th of six velocity difference threshold used as;	

    min_vectors_2d	value = 3;	
			name = Min # Vectors/2D Feature;	
			type = int;		
			range = [1, 10];	
			description = Minimum number of pattern vectors required to declare;	

    vector_rad_dist_2d	value = 0.5;	
			name = 2D Vector Radial Distance;	
			type = double;		
			range = [0.0, 3.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum radial distance between two pattern vectors;	

    vector_azi_dist_2d	value = 1.5;	
			name = 2D Vector Azimuthal Distance;	
			type = double;		
			range = [0.0, 4.0];	
			accuracy = [0.1];
			unit = degrees;		
			description = Maximum azimuthal distance between two pattern vectors;	

    max_ratio_2d	value = 4.0;	
			name = 2D Feature Aspect Ratio;	
			type = double;		
			range = [1.0, 10.0];	
			accuracy = [0.1];
			unit = ratio;		
			description = Maximum allowable aspect ratio (delta slant;	

    circ_radius1	value = 2.5;	
			name = Circulation Radius 1;	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = km;		
			description = The maximum horizontal radius used for searching for;	

    circ_radius2	value = 4.0;	
			name = Circulation Radius 2;	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = km;		
			description = The maximum horizontal radius used for searching for;	

    circ_radius_range	value = 80;	
			name = Circulation Radius Range;	
			type = int;		
			range = [1, 230];	
			unit = km;		
			description = Range(Slant) beyond which circulation radius 2;	

    max_2d_features	value = 600;	
			name = Max # 2D Features;	
			type = int;		
			range = [600, 800];	
			description = Maximum number of 2-D features the algorithm can;	

    min_2d_features	value = 3;	
			name = Min # 2D Features/3D Feature;	
			type = int;		
			range = [1, 10];	
			description = Minimum number of 2-D features needed to make a 3-D;	

    min_depth_3d	value = 1.5;	
			name = Min 3D Feature Depth;	
			type = double;		
			range = [0.0, 5.0];	
			accuracy = [0.1];
			unit = km;		
			permission = [ROC, URC];
			description = The minimum depth required to declare a TVS.  Shallower 3D features tend to be false alarms because they don’t have proper vertical continuity.;

    min_vel_3d		value = 25;	
			name = Min 3D Feat Low-Lvl Delta Vel;	
			type = int;		
			range = [0, 100];	
			unit = m/s;		
			permission = [ROC, URC];
			description = The minimum radial velocity difference at the base elevation scan required to declare a TVS.  Lower values will lead to more false alarm detections.;

    min_tvs_vel		value = 36;	
			name = Min TVS Delta Velocity;	
			type = int;		
			range = [0, 100];	
			unit = m/s;		
			permission = [ROC, URC];
			description = The minimum radial velocity difference of the maximum 3D Feature delta velocity required to declare a TVS detection.  Smaller values will lead to more false alarm detections.;

    max_3d_features	value = 35;	
			name = Max # 3D Features;	
			type = int;		
			range = [30, 50];	
			description = Maximum number of 3-D features the algorithm can;	

    max_tvs_features	value = 15;	
			name = Max # TVS's;	
			type = int;		
			range = [15, 25];	
			description = Maximum number of TVS's the algorithm can process;	

    max_etvs_features	value = 0;	
			name = Max # Elevated TVS's;	
			type = int;		
			range = [0, 25];	
			permission = [ROC, URC];
			description = Maximum number of Elevated TVS’s the algorithm can process per volume scan.  This feature has been turned off and set to 0 since the algorithm was first fielded.;

    min_tvs_height	value = 0.6;	
			name = Min TVS Base Height;	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = km;		
			description = Minimum height AGL to which the base of a 3-D;	

    min_tvs_elev	value = 1.0;	
			name = Min TVS Elevation;	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = degrees;		
			description = Lowest elevation angle to which the base of a 3-D;	

    avg_vel_height	value = 3.0;	
			name = Min Avg Delta Velocity Height;	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = km;		
			description = Minimum height AGL below which all 2D circulations;	

    max_storm_dist	value = 20.0;	
			name = Max Storm Association Dist.;	
			type = double;		
			range = [0.0, 20.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum distance from a storm within which to;	

