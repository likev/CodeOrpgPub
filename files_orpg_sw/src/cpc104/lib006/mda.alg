# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/12 20:57:06 $
# $Id: mda.alg,v 1.9 2010/05/12 20:57:06 ccalvert Exp $
# $Revision: 1.9 $
# $State: Exp $

    alg_name 		value = MDA

    min_refl		value = 0;	
			name = Minimum Reflectivity;	
			type = int;		
			range = [-25, 35];	
			unit = dBZ;
			permission = [ROC, URC];		
			description = Minimum reflectivity (dBZ) needed to process radial velocity data.  Larger values avoid false alarms in weak reflectivity regions.;

    overlap_filter_on	value = Yes;	
			name = Overlap Display Filter;	
			type = string;		
			range = { No, Yes};	
			permission = [ROC, URC];		
			description = Boolean value that allows a user to turn on the algorithm based overlap display filter. The overlap display filter eliminates the display of 3D features that overlap lower elevation features so that lower elevation features are not covered up.;
			enum =  0, 1;	

    min_filter_rank	value = 5;	
			name = Minimum Display Filter Rank;	
			type = int;		
			range = [1, 5];	
			permission = [ROC, URC];		
			description = The minimum strength rank below which 3D Features are marked not to be displayed.;

    beam_width		value = 1.0;	
			name = Beam Width;	
			type = float;
			unit = deg;
			accuracy = [0.1];		
			range = [0.0, 10.0];	
			description = Beam width of the radar.  Used by mda3d.;
			
    meso_min_nsv        value = 4;
    			name = Minimum 1D Shear Features in a 2D Feature;
			type = int;		
			range = [3, 8];	
			description = Minimum number of 1D shear features needed to declare a 2D feature. Used by mda2d.;

    meso_vs_rng_1       value = 100;
    			name = Beginning Range Linear Reduction;
			type = int;
			unit = km;
			range = [0, 230];	
			description = Beginning slant range at which the velocity difference and shear thresholds start to decrease linearly. Used by mda1d, mda2d and mda3d.;
			
    meso_vs_rng_2       value = 200;
    			name = Ending Range Linear Reduction;
			type = int;
			unit = km;
			range = [0, 230];	
			description = Ending slant range at which the velocity difference and shear thresholds start to decrease linearly. Used by mda1d, mda2d and mda3d.;

    meso_max_vect_len   value = 10;
    			name = Maximum 1D Shear Feature Length;
			type = int;
			unit = km;
			range = [5, 15];	
			description = Maximum length of a 1D shear feature not having a core 1D shear feature within it. Used by mda1d and mda2d.;

    meso_max_core_len   value = 10;
    			name = Maximum Core 1D Shear Feature Length;
			type = int;
			unit = km;
			range = [5, 15];	
			description = Maximum length of a core 1D shear feature within a 1D shear feature that exceeds meso_max_vect_len. Used by mda1d.;
			
    meso_max_ratio	value = 2.0;	
			name = Maximum 2D Feature Aspect Ratio;	
			type = float;
			accuracy = [0.5];		
			range = [1.0, 4.0];	
			description = For a 2D feature, the maximum allowable ratio of slant range size divided by azimuthal size. Used by mda2d.;

    meso_min_ratio	value = 0.0;	
			name = Minimum 2D Feature Aspect Ratio;	
			type = float;
			accuracy = [0.5];		
			range = [0.0, 4.0];	
			description = For a 2D feature, the minimum allowable ratio of slant range size divided by azimuthal size. Used by mda2d.;

    meso_min_radim	value = 1.00;	
			name = Minimum 2D Feature Radial Diameter;	
			type = float;
			unit = km;
			accuracy = [0.05];		
			range = [0.75, 2.50];	
			description = For a 2D feature, the minimum allowable difference between its 1D shear features having the minimum and maximum center ranges. Used by mda2d.;
			
    meso_max_dia	value = 15;	
			name = Maximum 2D Feature Diameter;	
			type = int;
			unit = km;
			range = [10, 20];	
			description = For a 2D feature, the maximum allowable difference between the location of its minimum and maximum velocities. Used by mda2d.;

    meso_2d_dist	value = 1.00;	
			name = Maximum 2D Construction Distance;	
			type = float;
			unit = km;
			accuracy = [0.25];		
			range = [0.25, 2.00];	
			description = The maximum allowable radial distance between 1D shear features to become associated into a 2D feature. Used by mda2d.;
			
    meso_min_rng	value = 5;	
			name = Minimum Range of a 2D Feature;	
			type = int;
			unit = km;
			range = [0, 15];	
			description = The minimum allowable distance from the radar to the center of a 2D feature. Used by mda3d.;

    v_d_th_lo           value = 2;	
			name = Minimum 2D Association Distance;	
			type = int;
			unit = km;
			range = [2, 10];	
			description = The minimum search radius used to when associating 2D features into 3D features. Used by mda3d.;
			
    v_d_th_hi           value = 8;	
			name = Maximum 2D Association Distance;	
			type = int;
			unit = km;
			range = [5, 10];	
			description = The maximum search radius used to when associating 2D features into 3D features. Used by mda3d.;

    conv_max_lookahd    value = 4;	
			name = Maximum Convergence Vector Lookahead Bins;	
			type = int;
			range = [3, 6];	
			description = The maximum number of bins to look ahead when building 1D convergence vectors. Used by mda1d.;

    conv_rng_dist       value = 1.0;	
			name = Minimum Convergence Vector Length;
			type = float;
			unit = km;
			accuracy = [0.5];
			range = [1.0, 2.0];	
			description = Minimum distance between minimum and maximum velocity values in a 1D convergence vector. Used by mda1d.;

    meso_conv_buff      value = 2;	
			name = Convergence Vector Buffer Zone;	
			type = int;
			unit = km;
			range = [0, 4];	
			description = Distance outside the diameter of a 2D feature in which 1D convergence vectors are included in the 2D feature convergence calculations. Used by mda2d.;

    meso_ll_conv_ht     value = 2;	
			name = Maximum Height for 3D Feature Low Level Convergence;
			type = int;
			unit = km;
			range = [1, 3];	
			description = Maximum height for calculating low level convergence for 3D features. Used by mda3d.;

    meso_ml_conv_ht1    value = 2;	
			name = Minimum Height for Mid Level Convergence;	
			type = int;
			unit = km;
			range = [2, 4];	
			description = Minimum height for calculating mid level convergence for 3D features. Used by mda3d.;
			
    meso_ml_conv_ht2    value = 4;	
			name = Maximum Height for Mid Level Convergence;	
			type = int;
			unit = km;
			range = [3, 6];	
			description = Maximum height for calculating mid level convergence for 3D features. Used by mda3d.;

    mda_no_shallow      value = 1;	
			name = Shallow 3D Features Not Allowed;	
			type = int;
			range = [0, 1];	
			description = Flag set to disable the reporting of shallow 3D features. Used by mda3d.;

    meso_min_rank_shal  value = 5;	
			name = Minimum Shallow Strength Rank;	
			type = int;
			range = [2, 5];	
			description = Minimum strength rank required for a 3D feature to be classified as shallow. Used by mda3d.;
		
    meso_min_depth_shal value = 1.0;	
			name = Minimum Shallow Depth;	
			type = float;
			unit = km;
			accuracy = [0.5];
			range = [0.5, 1.5];	
			description = Minimum depth required for a 3D feature to be classified as shallow. Used by mda3d.;

    meso_max_top_shal   value = 3;	
			name = Maximum Shallow Top;	
			type = int;
			unit = km;
			range = [1, 3];	
			description = Maximum top height permitted for a 3D feature to be classified as shallow. Used by mda3d.;
