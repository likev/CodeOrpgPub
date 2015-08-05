/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/03/21 20:55:49 $
 * $Id: saaConstants.h,v 1.2 2005/03/21 20:55:49 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef _SAACONSTANTS_H_
#define _SAACONSTANTS_H_
 
#define MAX_AZM 360       /* Number of whole degree                   */
#define MAX_RNG 230       /* Maximum number of input base data radials*/
#define MAX_TILTS 20	  /*  Maximum number of elevation angles	*/
#define RDMSNG 256        /* Missing data value for biased dBZ        */
#define BLOCK_RAD 3600    /* Number of tenths of a degree             */
#define SEC_IN_HOUR 3600  /* seconds in one hour                      */
#define SEC_IN_DAY 86400  /* seconds in one day                       */
#define SEC_IN_MIN 60     /* seconds in one minute                    */
#define MIN_IN_HOUR 60     /* minutes in one hour                    */
#define HOURS_IN_DAY 24	  /* hours in one day			      */
#define ZERO 0            /* Constant value                           */
#define FALSE 0
#define TRUE  1
#define FLAG_SET 1        /* Constant parameter for a set flag        */
#define FLAG_CLEAR 0      /* Constant parameter for a set flag        */
#define SAA_DEBUG 0       /* Flag for debug printing.  TODO: change this
			     to 0 on release builds */

static const float EARTH_RADIUS		= 6370.0;
static const float PROP_FCTR	    	= 1.21;
static const float SAA_ACCUM_CUTOFF	= 32767.0;      /*Cap for accumulation */


#endif /* _SAACONSTANTS_H_  */
