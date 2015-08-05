/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:49 $
 * $Id: mode_select.h,v 1.10 2007/01/30 22:56:49 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*	This header file defines the structure for the precipitation	*
 *	detection/mode selection function.  It corresponds to common	*
 *	block PRECIP_DETECT in the legacy code.				*/



#ifndef MODE_SELECT_H
#define	MODE_SELECT_H

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
      INTEGER NUMCATS
      PARAMETER (NUMCATS=6)
      INTEGER MODE_SELECT( NUMCATS )
C
C
      COMMON /MODE_SELECT/ MODE_SELECT
C
**********************************************************************/
#include <orpgctype.h>

#define	MODE_SELECT_DEA_NAME "alg.mode_select"

#define MODE_SELECT_NUMCATS               10

/*	Definitions for valid field range limits			*/

#define	MODE_SELECT_DBR_MIN		-30.0	/*  Precip Rate		*/
#define	MODE_SELECT_DBR_MAX		 50.0	/*  Precip Rate		*/
#define	MODE_SELECT_AREA_MIN		    0	/*  Nominal/Precip Area	*/
#define	MODE_SELECT_AREA_MAX		80000	/*  Nominal/Precip Area	*/
#define	MODE_SELECT_CAT_MIN		    0	/*  Precip Category	*/
#define	MODE_SELECT_CAT_MAX		    1	/*  Precip Category	*/


typedef struct{

	freal precip_mode_zthresh;			

	fint precip_mode_area_thresh;

	fint auto_mode_A;

	fint auto_mode_B;

	fint mode_B_selection_time;

        fint ignore_mode_conflict;

        fint mode_conflict_duration;

        fint use_hybrid_scan;

        fint clutter_thresh;

        fint spare;

} Mode_select_entry_t;


typedef struct {

	fint	wxstatus_select [MODE_SELECT_NUMCATS];
				/* MODE_SELECT */
		/***********************************************************************
		 *  mode_select [0] = Precip mode dBZ threshold, in 0.1 dBZ.
		 *  mode_select [1] = Precip area threshold, in km**2.
		 *  mode_select [2] = Auto Mode A flag.
		 *  mode_select [3] = Auto Mode B flag.
		 *  mode_select [4] = Mode B Selection Time, in mins.
		 *  mode_select [5] = Ignore Mode Conflict flag.
		 *  mode_select [6] = Mode Conflict Duration, in hours.
		 *  mode_select [7] = Use Hybrid Scan as input flag.
		 *  mode_select [8] = Clutter Likelihood threshold, in %.
		 *  mode_select [9] = Spare.                                 
		 **********************************************************************/

} Mode_select_t;

#endif
