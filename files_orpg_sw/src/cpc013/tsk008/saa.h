/*
 * RCS info
 * $Author: aamirn $
 * $Locker:  $
 * $Date: 2008/01/04 20:54:41 $
 * $Id: saa.h,v 1.3 2008/01/04 20:54:41 aamirn Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


/*******************************************************************************  
    File   : saa.h
    Created: Aug. 7, 2003
    The Snow Accumulation Algorithm (SAA) processes Hybrid Scan reflectivity and elevation angle 
    files generated by the EPRE algorithm and computes snow and liquid water equivalent (LWE)
    accumulations.  The logic is similar to the Precipitation Processing System (PPS).  
    Originally from Tim O'Bannon, June 2003
    
 *******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input_buffer_struct.h" /* file that defines the input buffer structure */
#include "saaConstants.h"


#include "saa_adapt.h"
#include <saa_file_names.h>


/*****************************************************************/
/*Data Declarations*/

float previous_ZS_mult;/*kd added 12/29 to kep track of the previously_used cf_ZS_mult */
float previous_ZS_power;/*added 12/29 to keep track of the previously_used cf_ZS_power */


/* Temporary structure for the adaptation data (defined in saa_adapt.h) */
saa_adapt_t saa_adapt;

/* Matrices to hold snow rate, scan_to_scan_swe, and scan_to_scan_sd */
float  snow_rate[MAX_AZM][MAX_RNG],
       previous_snow_rate[MAX_AZM][MAX_RNG], /* required for extrapolation */
       scan_to_scan_swe[MAX_AZM][MAX_RNG],   /* scan-to-scan snow water equivalent */
       scan_to_scan_sd[MAX_AZM][MAX_RNG];    /* scan-to-scan snow depth */
      
/*****************************************************************/
/*Function Declarations*/

int saa();
void read_saa_adaptation();
