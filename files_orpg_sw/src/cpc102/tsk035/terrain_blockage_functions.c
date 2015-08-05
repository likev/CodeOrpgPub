/*************************************************************************

   Module:  mnttsk_hydromet.c

   Description:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/08/03 21:28:00 $
 * $Id: terrain_blockage_functions.c,v 1.1 2005/08/03 21:28:00 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <stdio.h>
#include <stdlib.h>            /* EXIT_SUCCESS, EXIT_FAILURE              */
#include <unistd.h>            /* unlink()                                */
#include <errno.h>
#include <sys/types.h>         /* open(),lseek()                          */
#include <sys/stat.h>          /* open()                                  */
#include <fcntl.h>             /* open()                                  */
#include <math.h>

#include <orpg.h>
#include <hydro_files.h>
#include <orpgsite.h>

/*
  Static Function Prototypes. 
*/
static int Beam_blockage(void);
static int get_terrain_hts(void);      /*  Get terrain height information.                     */
static void conv_2_elev_ang(void);     /*  Compute the radar beam/terrain interception angle.  */
static float radians_2_degrees(float); /*  Convert from radians to degrees.                    */
static void build_beam_wt_table(void); /*  Build Gaussian beam weight table.                   */
static int compute_blockage(int);      /*  Compute beam blockage.                              */
static void write_blockage_to_lb(int); /*  Write beam blockage to linear buffer.               */


/*****************************************************************************

   Description:
	Initializes the Beam Blockage files.

   Inputs: 

   Outputs:
   
   Returns:
	Negative value on error, or 0 on success.

*****************************************************************************/
int Initialize_beam_blockage(void){

    int retval ;

    /*
     * Run algorithm to initialize beam blockage file.  
     */
    LE_send_msg( GL_INFO, "Perform Initialization of Blockage Data\n" );

    /* Run beam blockage algorithm.  */
    if ((retval = Beam_blockage()) < 0) {
          LE_send_msg(GL_INFO, "Beam Blockage Algorithm failure (%d)", retval);
          return(retval);
    }

    return(0) ;

/*END of Initialize_beam_blockage()*/
}

/****************************************************************************

   Description:
	Estimate radar beam blockage by mapping a simulated radar beam pattern 
	against the site specific terrain elevation data using standard beam 
	propagation.  The beam blockage (in percent) is generated for each 0.1 
	degree elevation increment from MIN_ELEV through the highest elevation 
	with beam blockage (but not exceeding MAX_ELEV).  For each elevation 
	angle, the beam blockage is computed in a 3600 (0.1 degree azimuth) by 
	230 (1 kilometer range) array.   The blockage information will be used 
	by the PPS Enhanced Preprocessing (EPRE) algorithm.

   Inputs:

   Outputs:

   Returns:
	Exits with non-zero exit code on error, or returns 0 on success.
      
   Author:
   	All modules - Tim O'Bannon, ROC Applications Branch, June 2002

*****************************************************************************/

int	interceptEl[MAX_AZIMS][MAX_RANGE];	/*  Radar elevation angle that would intercept the 
						    terrain at the azimuth and range (in tenths of
						    a degree).	*/

float	beamWtTable[BWT_ARRAY_SIZE][BWT_ARRAY_SIZE]; 	/*  Beam weights used to emulate the Gaussian
							    distribution.  This is applied to the 
							    terrain to determine blockage for each 
							    0.1 degree in azimuth.	*/

char 	pctBlockage[MAX_AZIMS][MAX_RANGE];      /*  Beam blockage at each 0.1 degree azimuth and 
						    1 km range. */

terrain_data_t *terrainHt = NULL;

static int Beam_blockage() {

	int	elevAngle,
		errMsg;	

        /* Tell the operator this may take awhile. */
	LE_send_msg(GL_INFO, "Building Beam Blockage information.\n");
	LE_send_msg(GL_INFO, "NOTE: THIS MAY TAKE A FEW MINUTES.\n");

        LE_send_msg(GL_INFO, "Getting terrain heights\n" ); 
	if ((errMsg = get_terrain_hts()) < 0) {
		LE_send_msg(GL_INFO, "get_terrain_hts failed: (%d)");
		return(errMsg);
	}

        LE_send_msg(GL_INFO, "Building Gaussian beam weight table.\n" ); 
	build_beam_wt_table();

        LE_send_msg(GL_INFO, "Computing blockage.\n" ); 
	for (elevAngle = MIN_ELEV; elevAngle < MAX_ELEV; elevAngle++) 
		if (compute_blockage(elevAngle) == 0) break;
	
	LE_send_msg(GL_INFO, "Completed Beam Blockage Algorithm.\n" ); 
	
	return(0);
}

/****************************************************************************

   Description:
	Reads in the terrain height information and, for each range and azimuth 
	angle, computes the elevation angle (in tenths of a degree) at which 
	the radar beam would intercept the terrain (assuming the propagation is 
	represented by a standard earth atmosphere).  The interception elevation 
	angle is later compared with the beam elevation angle to compute beam 
	blockage.  
	
   Inputs:
	Site specific file containing the terrain height information in polar
	format.   It is derived from high resolution Digital Elevation Model
	(DEM) data.  In offline WSR-88D software, the rectangular latitude/
	longitude gridded DEM data (current resolution 3 by 3 arc seconds, or 
	approximately 90 by 90 meters) is transposed to polar format (0.1 degree 
	by 1 kilometer) centered at the radar.

   Outputs:
	The terrain intercept elevation angle.  This is the elevation angle 
	from the center of the radar antenna to the highest point within each 
	(1 km) range bin. 

   Returns:
	Exits with non-zero exit code on error, or returns 0 on success.
  
*****************************************************************************/

size_t	sizeTest;

static int get_terrain_hts() {
	
        char    *buf;
        
	int 	azAngle,
		rngBin,
		retVal;
		
/*  Initialize terrain height and intercept elevation angle data 	*/
	for (azAngle = 0; azAngle < MAX_AZIMS; azAngle++) 
		for (rngBin = 0; rngBin < MAX_RANGE; rngBin++) {
			interceptEl[azAngle][rngBin] = -900;
		}

/*  Read in the terrain height data from linear buffer	*/
	if ((retVal = ORPGDA_read(ORPGDAT_TERRAIN, &buf, LB_ALLOC_BUF, 1)) < 0) {
		LE_send_msg(GL_INFO, "ORPGDA_read of ORPGDAT_TERRAIN failed: %d\n",  retVal);
		return(-1);
	}
	else{
		LE_send_msg(GL_INFO, "ORPGDAT_TERRAIN has been read.");
		terrainHt = (terrain_data_t *) buf;
	}

/*  Compute terrain intercept elevation angle in tenths of a degree  */
	conv_2_elev_ang();

        free(buf);
	return(0);
}

/****************************************************************************

   Description:
	Computes the terrain intercept elevation angle for each range bin.  
	This routine uses the following calculation to determine the 
	intercept angle for a terrain height at a range:

	    Intercept angle = arctanget(htDif / Range - Range / (2 * 1.21 * Er))

	    where: htDif = difference between radar height and terrain height
		   Er = the mean radius of the earth (6371 km)
    	
	This calculation is derived from the following formula:

	    Ht = Range * tanget(elevation angle) + Range**2 / 2 * 1.21 * Er

	
   Inputs:

   Outputs:

   Returns:
      Exits with non-zero exit code on error, or returns 0 on success.
  
*****************************************************************************/

static void conv_2_elev_ang() {
	
	int 	azAngle,	/*  Azimuth angle in tenths of a degree	*/
		tRngBin;	/*  Range to the terrain height bin 	*/
	
	float	adjustedRadius = EARTH_RADIUS * REFRACTIVE_INDEX,
		rngKm,		/*  Range used to calculate intercept angle. 	*/
		htDif,		/*  Difference between radar height and terrain height (km) 	*/
		elevRadians,	/*  Radar elevation angle that would intersect the terrain (radians)	*/
		siteHtKm;  	/*  Site height (km)	*/
				
	siteHtKm = ORPGSITE_get_int_prop(ORPGSITE_RDA_ELEVATION) * 12. * 25.4 / 1000000.;
	LE_send_msg(GL_INFO, "Site height: %6.3f km\n",siteHtKm);

	for (azAngle = 0; azAngle < MAX_AZIMS; azAngle++) {
     	       for (tRngBin = 0; tRngBin < MAX_RANGE; tRngBin++) {
	
/*  Compute the difference between the height of the radar and the terrain height (in kilometers)  */	
			htDif = (terrainHt->height[azAngle][tRngBin] / 1000.) -  siteHtKm;

/*  Compute the range used to calculate intercept angle.  Use the midpoint of the range bin */

				rngKm = tRngBin + 0.5;	
						
/*  Compute the terrain intercept elevation angle  	*/
			elevRadians = atan(htDif / rngKm - rngKm / (2.0 * adjustedRadius));
			interceptEl[azAngle][tRngBin] = radians_2_degrees(elevRadians) * 10;

/*  For blockage computation, the terrain elevation angle intercepted by the beam cannot decrease
    with range, i.e. the radar cannot see behind mountains, so ensure that the interceptEl array 
    is monotonically increasing. 			*/ 
			if (tRngBin > 0 && 
				interceptEl[azAngle][tRngBin] < interceptEl[azAngle][tRngBin-1])
					interceptEl[azAngle][tRngBin] = interceptEl[azAngle][tRngBin-1];	

		}
	}
}

/****************************************************************************

   Description:
	Convert from radians to degrees. 
	
   Inputs:
 	Angle in radians.

   Outputs:
   
   Returns:
   	Angle in degrees.
   
*****************************************************************************/

static float radians_2_degrees(float radians) {
 
 	float 	pi = 3.14159265,
 		degrees;

	degrees = radians * 180.0 / pi;
	
	return(degrees);
}

/****************************************************************************

   Description:
	Build the beam weighting lookup table.  This function assumes the 
	beam is represented by a circular Gaussian distribution and uses the 
	mathematical formula defined in page 3-81 of the CPCI26 B-specs.  
	This function computes the fraction of the beam power in each 0.1 
	degree element of a BWT_ARRAY_SIZE by BWT_ARRAY_SIZE element array, 
	representing the total portion of the beam power within +/- 1.5 
	degrees. 

	To facilitate the summation of beam blockage, the total beam blockage 
	is computed for each 3 degree column for each azimuthal increment of 
	0.1 degrees. 
	
	The result of this function is a beamWtTable that contains the percent 
	beam blockage over a 3 by 3 degree region surrounding the beam center 
	for each azimuth and elevation angle in 0.1 degree increments.  
	
   Inputs:

   Outputs:
   
   Returns:
   
*****************************************************************************/
static void build_beam_wt_table() {
	
	int 	element,			/*  Array element 0 -> 30 	*/
		begAz = - BWT_ARRAY_SIZE / 2,	/*  Tenths of a degree  	*/
		endAz = BWT_ARRAY_SIZE / 2,	/*  Tenths of a degree  	*/
		deltaAz,
		row,
		column;	

	float 	horizIncr[BWT_ARRAY_SIZE],
		vertIncr[BWT_ARRAY_SIZE],
		sumHorizBmWt = 0.0,
		sumVertBmWt = 0.0,
		normalize,
		rowSum,
		ln2 = log(2.0),
		sigmaFn;
		
/*  Compute the vertical and horizontal gaussian distribution weighting factors.  The product of the
    sums of the vertical and horizontal factors will be used to normalize the 0.1 degree values in
    the 3X3 degree array.	*/	
	for (deltaAz = begAz; deltaAz <= endAz; deltaAz++) {
		element = deltaAz + endAz;	
		sigmaFn = (deltaAz / 10.0) / BEAM_WIDTH;
		horizIncr[element] = exp(HORIZ_SHAPE * ln2 * sigmaFn * sigmaFn);
		vertIncr[element] = exp(VERT_SHAPE * ln2 * sigmaFn * sigmaFn);
		sumHorizBmWt+= horizIncr[element];
		sumVertBmWt+= vertIncr[element];
	}

	normalize = sumHorizBmWt * sumVertBmWt;
		
/*  Build the 31X31 element 2D beam weighting lookup table.  For each bin, multiply the horizontal
    weight by the vertical weight.  Then normalize the weights (to ensure the total beam weight equals
    1.0) and sum the weights in each column to facilitate operational weighting.	*/	
	for (column = 0; column < BWT_ARRAY_SIZE; column++) {
		rowSum = 0.0;
		for (row = 0; row < BWT_ARRAY_SIZE; row++) {
			rowSum+= (horizIncr[row] * vertIncr[column]) / normalize;
			beamWtTable[row][column] = rowSum;  
		}
	}
}


/****************************************************************************

   Description:
	Uses the intercept elevation data, the radar beam pattern, and beam 
	elevation angle to calculate the beam blockage (in percent) for each 
	(0.1 degree) azimuth and (1 km) range bin for each tilt.  This is 
	performed by comparing the beam elevation angle from the radar with 
	the elevation angle of the terrain blockage by passing the Gaussian 
	weighted beam pattern over the terrain elevation angles.
	
   Inputs:
	Beam elevation angle (in tenths of a degree), intercept elevation 
	angle, and Gaussian beam weight table.

   Outputs:
	Beam blockage (in percent).
	
   Returns:
	A flag (anyBlockage) that indicates if any blockage was noted at this 
	elevation angle.  The beam blockage algorithm terminates if there is 
	no beam blockage.
   
*****************************************************************************/
static int compute_blockage(int beamElevAngle) {
	
	int 	rowNum, 	/*  Array row number, associated with elev angle difference 	*/
		terrainEl,
		azimAngle, 
		rangeBin, 
		colNum, 	/*  Array column number	*/
		hiAngle, 
		loAngle,
		colAzAngle,	/*  Azimuth angle lined up with weight array column  		*/
		arrayOffset = BWT_ARRAY_SIZE / 2,
		anyBlockage;	/*  Check to see if there is any blockage in the array	*/
		
	float	psum;
		
/*  Set elevation angle limits for the beam pattern array  */
	hiAngle = beamElevAngle + arrayOffset;
	loAngle = beamElevAngle - arrayOffset;

/*  Initialize blockage counter	 */
	anyBlockage = 0;
	
/*  Compute radar beam blockage  */
	for (azimAngle = 0; azimAngle < MAX_AZIMS; azimAngle++) 
		for (rangeBin = 0; rangeBin < MAX_RANGE; rangeBin++) { 	
			psum = 0.0;	/*  initialize summation variable  */

/*  Define the array column azimuthal locations.  	*/
			for (colNum = 0; colNum < BWT_ARRAY_SIZE; colNum++) { 
				colAzAngle = azimAngle - arrayOffset + colNum;
				
/*  Get the terrain elevation angle (in tenths of a degree) for each array column, correcting for
    crossover at 360 degrees azimuth. 	*/
				if (colAzAngle < 0) 
					terrainEl = interceptEl[colAzAngle+MAX_AZIMS][rangeBin];
				else if (colAzAngle < MAX_AZIMS)
					terrainEl = interceptEl[colAzAngle][rangeBin];
				else /* colAzAngle >= MAX_AZIMS */ 
					terrainEl = interceptEl[colAzAngle-MAX_AZIMS][rangeBin];
				
/*  Determine the row number from 0 to 30, based on the difference between the 
    beam elevation and the terrain elevation angle.  	*/
				if (terrainEl <= loAngle) 
					rowNum = 0;
				else if (terrainEl >= hiAngle) 
					rowNum = BWT_ARRAY_SIZE-1;
				else  /*  loAngle < terrainEl < hiAngle  */
					rowNum = terrainEl - beamElevAngle + arrayOffset;
			
/*  Add the beam weights of the rowNum for all the array columns to calculate beam blockage 	*/
				psum+= beamWtTable[rowNum][colNum];
			}
			
			pctBlockage[azimAngle][rangeBin] = psum * 100.;   /*  In percent  */
			if (pctBlockage[azimAngle][rangeBin] > 0) anyBlockage = 1;
		}

/*  Write blockage file to linear buffer.	*/
	write_blockage_to_lb(beamElevAngle); 
	return(anyBlockage);
}

/****************************************************************************

   Description:
	Writes the radar beam blockage file to a linear buffer.

   Inputs:
   	Beam elevation angle (in tenths of a degree).

   Outputs:
   	Beam blockage information for a single elevation angle.
   
   Returns:
   
*****************************************************************************/
size_t	sizeTest;

static void write_blockage_to_lb(int elevAngle) {

	int	arraySize = MAX_AZIMS * MAX_RANGE,
		retVal,
		
/*  For the beam blockage linear buffer, msgid is used to indicate the elevation angle (in
    tenths of a degree) of the beam blockage information.  However, elevation angles may 
    range from -1.0 to +20.0 degrees and msgid cannot be negative.  The variable elevOffset 
    is added to elevAngle to ensure that msgid is never negative.	*/
    		msgid,
		elevOffset = 10; 	
	
	msgid = elevAngle + elevOffset;
	
	if ((retVal = ORPGDA_write_permission(ORPGDAT_BLOCKAGE)) < 0)
		LE_send_msg(GL_INFO, "ORPGDA_write_permission failed: %d != %d",retVal);

	else if ((retVal = ORPGDA_write(ORPGDAT_BLOCKAGE, (char *) pctBlockage, arraySize, msgid)) < 0) 
		LE_send_msg(GL_INFO, "ORPGDA_write(BLOCKAGE msgid %d) failed: %d != %d",
			    msgid, retVal, arraySize);
	else 
		LE_send_msg(GL_INFO,"Data ID %d: (BLOCKAGE) msgid %d written.",
			    ORPGDAT_BLOCKAGE, msgid);

}
