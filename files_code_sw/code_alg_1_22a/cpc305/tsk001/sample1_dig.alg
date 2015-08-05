
# SAMPLE DEA FILE FOR CODE

# The first data element is an example of an enumeration
# Enumerations are typically used for flag values

    alg_name 		value = Sample 1 Digrefl

    db_Element_1	value = Yes;                 
			name = Flag (Element 1);             
			type = string;                       
			range = { No, Yes};                 
			description = Element 1 Description; 
			enum =  0, 1;	
# even though the type is a string, the enumerated value renurned
# when the data is read is an integer with value of 0 or 1


# The second data element is an example of a double type
# the permission attribute (if present) makes the element editable

    db_Element_2	value = 5.0;                  
			name = Threshold (Element 2);         
			type = double;                        
			range = [-12.0, 20.0];                
			unit = dBZ;                           
			description = Element 2 Description;
			permission = [ROC, URC];	
# The range is not inclusive ( -12.0 and 20.0) not within the range
# BUG - NON-INCLUSIVE RANGE DOES NOT WORK IN BUILD 5


# The third data element is an example of an integer type
# Default values can be assigned to specific sites, value unassigned

    db_Element_3	value = ;                    
			name = Threshold (Element 3);         
			type = int;                           
			range = [-4, 0];                      
			unit = bins;                        
			description = Element 3 Description;  
			permission = [ROC];
			default = KMLB:         0, \
			          Other_sites: -1;	
# The range is inclusive

