# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2015/05/27 19:51:16 $
# $Id: vdeal.alg,v 1.6 2015/05/27 19:51:16 steves Exp $
# $Revision: 1.6 $
# $State: Exp $


    alg_name		value = 2D velocity dealiasing

    solver_threads	value = 4;	
			name = Solver threads;	
			type = int;		
			description = Number of threads used in 2D solver;

    dealiase_threads	value = 8;	
			name = Dealiasing threads;	
			type = int;		
			description = Number of threads used in dealiasing partitions;

    max_partition_size	value = 56;	
			name = Max partition size;	
			range = [30, 800];	
			type = int;		
			description = Max partition size used in phase 2. A large number will result in higher dealiasing performance while requiring more powerful CPU for realtime processing;

    slow_processor	value = NO;	
			name = Set slow processor mode;	
			type = string;		
                        range = {YES, NO};
			description = The value of YES sets veldeal to run on a slow machine. This disables settings of max_partition_size, dealiase_threads and max_partition_size;

    hs_threshold	value = 20.0;	
			name = High shear threshold;	
			type = double;		
			description = Threshold for detecting high shear, in m/ s. If < Nyquist velocity or not specified, Nyquist velocity is used;

    hs_size		value = 12;	
			name = High shear feature size;	
			type = int;		
			description = The minimum size for high shear detection, in number of gates. If < 10 or not specified, 10 is used;
	
    b16_update1		value = 1;
			name = Apply B16 update 1;	
			type = int;		
			description = Applying B16 update 1, if 1, or disabling it otherwise;
 

