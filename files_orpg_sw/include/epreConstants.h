/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/12/07 19:02:11 $
 * $Id: epreConstants.h,v 1.2 2004/12/07 19:02:11 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef _EPRECONSTANTS_H_
#define _EPRECONSTANTS_H_
 
#define MAX_AZM 360       /* Number of whole degree                   */
#define MAX_RNG 230       /* Maximum number of input base data radials*/
#define MAX_RAD 400       /* Maximum number of input base data radials*/
#define RDMSNG 256        /* Missing data value for biased dBZ        */
#define BLOCK_RAD 3600    /* Number of tenths of a degree             */
#define SEC_IN_HOUR 3600  /* seconds in one hour                      */
#define SEC_IN_DAY 86400  /* seconds in one day                       */
#define SEC_IN_MIN 60     /* seconds in one minute                    */
#define ZERO 0            /* Constant value                           */

#ifndef NULL
#define NULL 0            /* Constant value                           */
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

#define FLAG_SET 1        /* Constant parameter for a set flag        */
#define FLAG_CLEAR 0      /* Constant parameter for a set flag        */

#endif /* _EPRECONSTANTS_H_  */
