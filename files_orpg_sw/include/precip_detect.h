/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:51 $
 * $Id: precip_detect.h,v 1.18 2007/01/30 22:56:51 ccalvert Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

/*	This header file defines the structure for the precipitation	*
 *	detection function.  It corresponds to common block		*
 *	PRECIP_DETECT in the legacy code.				*/



#ifndef PRECIP_DETECT_H
#define	PRECIP_DETECT_H

/********************************************************************
*.
*.           I N C L U D E    F I L E    P R O L O G U E
*.
*.  INCLUDE FILE NAME: A309ADPT.INC
*.
C*************************************************************
C  ALGORITHM ADAPTATION DATA
C*************************************************************
C
C   --Precip Detection Function
C
      INTEGER NUMROWS, NUMCATS
      PARAMETER (NUMROWS=4, NUMCATS=6)
      INTEGER PRECIP_THRESH( NUMROWS,NUMCATS )
C
C
      COMMON /PRECIP_DETECT/ PRECIP_THRESH
C
**********************************************************************/
#include <orpgctype.h>

#define	PRECIP_DETECT_DEA_NAME "precip_detect"

/*	Precip Detection Function	*/

#define	PRECIP_DETECT_NUMCATS		6
#define	PRECIP_DETECT_NUMROWS		4

/*	Definitions for valid field range limits			*/

#define PRECIP_THRESH_ELEV_MIN		 -1.0	/*  Min/Max Elev Angle	*/
#define	PRECIP_THRESH_ELEV_MAX		 45.0	/*  Min/Max Elev Angle	*/
#define	PRECIP_THRESH_DBR_MIN		-30.0	/*  Precip Rate		*/
#define	PRECIP_THRESH_DBR_MAX		 50.0	/*  Precip Rate		*/
#define	PRECIP_THRESH_AREA_MIN		    0	/*  Nominal/Precip Area	*/
#define	PRECIP_THRESH_AREA_MAX		80000	/*  Nominal/Precip Area	*/
#define	PRECIP_THRESH_CAT_MIN		    0	/*  Precip Category	*/
#define	PRECIP_THRESH_CAT_MAX		    2	/*  Precip Category	*/


/*    This stucture is used for the meta data dictionary  */
/*    Special code has been implemented in the dictionary to map the row-major order of
	the fields in the array to the column major order in the legacy common block 
      These structures do not currently match the binary contents of precip
      detection adaptation data.
*/
typedef struct	
{
	fint min_elev;			/*# @name "Min Elevation"
                                            @desc "Minimum elevation for this entry"
                                            @enum_values "No Entry=0", "Range:-1.0-45.0"
					    @units "degrees" @internal_units "deg * 10"
                                            @default 0
                                        */
	fint max_elev;			/*# @name "Max Elevation" @min -1.0 @max 45.0
					    @desc "Maximum elevation for this entry"
					    @units "degrees" @internal_units "deg * 10"
                                            @default 0
					*/
	fint rate;			/*# @name "Precipitation Rate" @min -30.0 @max 50.0
					    @desc "Precipitation rate for this entry"
					    @units "dBR" @internal_units "dBR * 10"
					    @default 0
					*/
	fint nominal_clutter_area;	/*#  @name "Nominal Clutter Area"
					     @desc "Clutter Area" @units "km**2"
					     @min 0   @max 80000
					*/
	fint precip_area_thresh;	/*   @name "Precip Area Threshold"
					     @desc "Precipitation area threshold"
					     @min 0 @max 80000 @units "km**2"
				        */
	fint precip_category;		/*   @name "Precipitation Category"
					     @enum_values "No Precip=0", "Significant Precip=1", "Light Precip=2"
					 */
} precip_detect_entry_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/
#ifdef __cplusplus
#endif



typedef struct {

	fint	precip_thresh [PRECIP_DETECT_NUMCATS][PRECIP_DETECT_NUMROWS];
				/* PRECIP_THRESH */
		/**********************************************************
		 *  precip_thresh [0][N] = Min elev angle, in 0.1 deg.
		 *  precip_thresh [1][N] = Max elev angle, in 0.1 deg.
		 *  precip_thresh [2][N] = Precip rate thrshld, in 0.1 dBR.
		 *  precip_thresh [3][N] = Nominal clutter area, in km**2.
		 *  precip_thresh [4][N] = Precip area thrshld, in km**2.
		 *  precip_thresh [5][N] = Precip category:
		 *				0 - No Precip
		 *				1 - Significant Precip
		 *				2 - Minimal (Light) Precip
		 *  NOTE:  A zero in the first and second columns
		 *         signifies a non-valid row.
		 **********************************************************/

} precip_detect_t;

typedef struct {	/* An improved version of precip_detect_t - This is 
			   binary-equivalent to precip_detect_t */

    int min_elev[PRECIP_DETECT_NUMROWS];    /* Min elev angle, in 0.1 deg */
    int max_elev[PRECIP_DETECT_NUMROWS];    /* Max elev angle, in 0.1 deg */
    int rate[PRECIP_DETECT_NUMROWS];    /* Precip rate thrshld, in 0.1 dBR */
    int nominal_clutter_area[PRECIP_DETECT_NUMROWS];    
					/* Nominal clutter area, in km**2 */
    int precip_area_thresh[PRECIP_DETECT_NUMROWS];    
					/* Precip area thrshld, in km**2 */
    int precip_category[PRECIP_DETECT_NUMROWS];    
					/* Precip category: 
					   0 - No Precip; 
					   1 - Significant Precip; 
					   2 - Minimal (Light) Precip */
} precip_detect1_t;		/* all unused elements have to be set to 0 */


/*#  Since the binary structure of the Precip Detection data is
     still bound to the legacy common block structure, special
     read/write methods are written to deal with the legacy
     common block format

     Declare a C++ wrapper class that statically contains a pointer to the meta
     data for this structure.  The C++ wrapper class allows the IOCProperty API
     to be implemented automatically for this structure
**/

#endif
