# RCS info
# $Author: steves $
# $Locker:  $
# $Date: 2014/03/26 13:56:34 $
# $Id: mode_select.alg,v 1.12 2014/03/26 13:56:34 steves Exp $
# $Revision: 1.12 $
# $State: Exp $

   alg_name					value = Mode Selection

   precip_mode_zthresh				value = 30.0;	
						name = Precip Mode dBZ Thresh;	
						type = double;		
						range = [-33.0, 95.0];
						accuracy = [ 0.1 ];
						unit = dBZ;		
						permission = [ROC, URC];
						description = Minimum reflectivity to be considered for changing from Clear Air Mode to Precipitation Mode.;

   precip_mode_area_thresh			value = 80;	
						name = Precip Mode dBZ Areal Coverage Thresh;
						type = int;		
						range = [0, 80000];
						unit = km^2;		
						permission = [ROC, URC];
						description = Minimum area of reflectivity greater than the Precip Mode dBZ threshold to be considered for changing from Clear Air Mode to Precipitation Mode.;

   mode_B_selection_time			value = 20;	
              		          		range = [0, 60];
						name = Clear Air Mode Selection Time;
						type = int;		
						unit = min;		
						permission = [ROC, URC];
						description = The delay required after Precipitation Mode conditions are no longer met before the system can change to the default Clear Air VCP.;

   auto_mode_A					value = Auto;	
						range = {Manual, Auto};
						enum = 0, 1;		
						name = Precipitation Switching;
						type = string;		
						misc = @-Not_for_alg_edit-@;
						permission = [ROC, URC];

   auto_mode_B					value = Auto;	
						range = {Manual, Auto};
						enum = 0, 1;		
						name = Clear Air Switching;
						type = string;		
						misc = @-Not_for_alg_edit-@;
						permission = [ROC, URC];

   mode_conflict_duration			value = 8;	
						range = [2, 48];
						name = Mode Conflict Duration;
						type = int;		
						unit = hr;		
						permission = [ROC, URC];
						description = The period of time that manual mode switch settings can remain in conflict with the MSF's recommended mode.;

   ignore_mode_conflict				value = No;	
						range = {No, Yes};
						enum = 0, 1;		
						name = Ignore Mode Conflict Duration?;
						type = string;		
						permission = [ROC, URC];
						description = If Yes, this setting will cause the Mode Conflict Duration to be ignored.;

   use_hybrid_scan				value = No;	
						range = {No, Yes};
						enum = 0, 1;		
						name = Use Hybrid Scan for MSF PP?;
						type = string;		

   clutter_thresh      				value = 50;
                        			name = Maximum Allowable % Likelihood of Clutter;
			                        type = int;
                       				range = [0, 100];
                        			unit = %;
			                        description = Maximum clutter likelihood to allow use of BASE;

