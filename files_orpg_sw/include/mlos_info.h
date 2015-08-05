/* 
 * RCS info
 * $Author: eforren
 * $Locker:  $
 * $Date: 2004/10/15 14:09:15 $
 * $Id: mlos_info.h,v 1.4 2004/10/15 14:09:15 ryans Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  
/************************************************************************
 *									*
 * 	Header defining mlos adaptation data				*	
 *									*
 ***********************************************************************/


#ifndef	MLOS_INFO_H
#define	MLOS_INFO_H

typedef enum
{
	MLOS_NONE=0,   		/*# @desc "None"   */
	MLOS_RPG_NON_DIV,       /*# @desc "RPG NON DIV"  */
	MLOS_RPG_DIV,		/*# @desc "RPG DIV" 	 */
	MLOS_RDA_NON_DIV,	/*# @desc "RDA NON DIV"	 */
	MLOS_RDA_DIV,		/*# @desc "RDA DIV"      */
	MLOS_RPT_NON_DIV,	/*# @desc "RPT DIV"      */
	MLOS_RPT_DIV,		/*# @desc "RPT ND/DIV"   */
	MLOS_RPG_ND_DIV,	/*# @desc "RPG ND/DIV"   */
} mlos_station_type_t;

typedef struct
{
	int no_of_mlos_stations;		/*#  @name "Number of MLOS stations"
						     @desc "Number of MLOS stations at this site"
						     @default 0  @min 0  @max 4
						*/
	mlos_station_type_t station_type[4];	/*#  @name "Station Type"
						     @desc "Type of MLOS station"
						*/
} mlos_info_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/
#ifdef __cplusplus
#endif
#endif
