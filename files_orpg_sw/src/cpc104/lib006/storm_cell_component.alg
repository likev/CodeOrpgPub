# RCS info
# $Author: ccalvert $
# $Locker:  $
# $Date: 2010/05/12 20:57:07 $
# $Id: storm_cell_component.alg,v 1.9 2010/05/12 20:57:07 ccalvert Exp $
# $Revision: 1.9 $
# $State: Exp $


    alg_name		value = Storm Cell Components

    segment_overlap	value = 2;	
			name = Thresh (Segment Overlap);	
			type = int;		
			range = [0, 5];	
			unit = bins;		
			description = Minimum overlap in sample volumes in order for segments to be identified as part of the same component.;	

    max_pot_comp_per_elev	value = 70;	
			name = Thresh (Max Pot Comps/Elv);	
			type = int;		
			range = [10, 100];	
			unit = storm components;	
			description = Maximum number of components thta can be defined in each elevation scan.;	

    max_comp_per_elev	value = 120;	
			name = Thresh (Max Comps/Elev);	
			type = int;		
			range = [20, 120];	
			unit = storm components;	
			description = Maximum number of storm cells that can be detected per volume scan (prior to deletion/merger.;	

    max_detect_cells	value = 130;	
			name = Thresh (Max Detected Cells);	
			type = int;		
			range = [20, 130];	
			unit = storm cells;		
			description = Maximum number of storm cells that can be detected per volume scan (prior to deletion/merger.);	

    num_segs_per_comp	value = 2;	
			name = Thresh (# Segments/Comp);	
			type = int;		
			range = [1, 4];	
			unit = storm segments;		
			description = Minimum number of segments required in a component.;	

    max_cells_per_vol	value = 100;	
			name = Thresh (Max Cells/Volume);	
			type = int;		
			range = [20, 100];	
			unit = storm cells;		
			description = Maximum number of cells that can be defined per volume scan.;	

    max_vil		value = 120;	
			name = Thresh (Max Cell Based VIL);	
			type = int;		
			range = [1, 120];	
			unit = kg/m^2;		
			permission = [ROC, URC];
			description = Upper limit for the maximum value for the cell-based Vertically Integrated Liquid (VIL) water (kg/m2).  Contamination of strong echo returns from hail will cause the VIL value to be overestimated.  The cell-based VIL because it uses storm components, typically has higher values than the original gridded VIL algorithm.;

    comp_area		value = 10.0, 10.0, 10.0, 10.0, 10.0, 10.0, 10.0;	
			name = Thresh - Component Area #;	
			type = double;		
			range = [10.0, 30.0];	
			accuracy = [0.1];
			unit = km^2;		
			description = Minimum area of component composed of segments identified with relevant threshold (reflectivity).;	

    search_radius	value = 5.0, 7.5, 10.0;	
			name = Thresh - Search Radius #;	
			type = double;		
			range = [1.0, 15.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum horizontal distance between two components' centroids on adjacent elevation angles for correlating into the same storm cell.;	

    depth_delete	value = 4.0;	
			name = Thresh (Depth Delete);	
			type = double;		
			range = [0.0, 10.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum difference in depths of two storm cells required to delete one of the storm cells;	

    horiz_delete	value = 5.0;	
			name = Thresh (Horizontal Delete);	
			type = double;		
			range = [3.0, 30.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum horizontal difference between two centroids required to delete one of the storm cells;	

    elev_merge		value = 3.0;	
			name = Thresh (Elevation Merge);	
			type = double;		
			range = [1.0, 5.0];	
			accuracy = [0.1];
			unit = degrees;		
			description = Maximum difference in elevation angles between the top of one storm cell and the bottom of another. Required to merge the storm cells.;	

    height_merge	value = 4.0;	
			name = Thresh (Height Merge);	
			type = double;		
			range = [1.0, 8.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum difference in height between the top of one storm cell and the bottom of another. Required to merge the storm cells.;	

    horiz_merge		value = 10.0;	
			name = Thresh (Horizontal Merge);	
			type = double;		
			range = [5.0, 20.0];	
			accuracy = [0.1];
			unit = km;		
			description = Maximum horizontal distance between two centroids. Required to merge the storm cells.;	

    azi_separation	value = 1.5;	
			name = Thresh (Az Separation);	
			type = double;		
			range = [1.5, 3.5];	
			accuracy = [0.1];
			unit = degrees;		
			description = Maximum azimuthal separation for segments to be identified as part of the same component.;	

