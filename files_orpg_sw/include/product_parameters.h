/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2007/01/30 22:56:53 $
 * $Id: product_parameters.h,v 1.16 2007/01/30 22:56:53 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */  
/**********************************************************************

	Header defining the data structures and constants used to
	define selectable product parameters.

**********************************************************************/


#ifndef	PRODUCT_PARAMETERS_H
#define PRODUCT_PARAMETERS_H

#define	PUP_ID_MNEMONIC_LENGTH		 4
#define	NUM_REFLECTIVITY_DATA_LEVELS	16
#define	NUM_VELOCITY_DATA_LEVELS	16
#define	NUM_VAD_RCM_DATA_LEVELS		70
#define	NUM_OHP_THP_DATA_LEVELS		16
#define	NUM_STP_DATA_LEVELS		16
#define	NUM_VELOCITY_DATA_LEVEL_TABLES	 8
#define	MAX_VAD_HEIGHTS			30
#define	MAX_RCM_HEIGHTS			19


#include <layer_prod_params.h>
#include <rcm_prod_params.h>
#include <cell_prod_params.h>

#define VAD_RCM_HEIGHTS_DEA_NAME	"vad_rcm_heights_t"
#define STP_DATA_LVLS_DEA_NAME		"STP_data_levels"
#define OHP_THP_DATA_LVLS_DEA_NAME	"OHP/THP_data_levels"
#define REF_DATA_LVLS_CLR16_DEA_NAME	"Reflectivity_data_levels_clear_16"
#define REF_DATA_LVLS_CLR8_DEA_NAME	"Reflectivity_data_levels_clear_8"
#define REF_DATA_LVLS_PRC16_DEA_NAME	"Reflectivity_data_levels_precip_16"
#define REF_DATA_LVLS_PRC8_DEA_NAME	"Reflectivity_data_levels_precip_8"
#define RCM_REF_DATA_LVLS_DEA_NAME	"RCM_reflectivity_data_levels"
#define VEL_DATA_LVL_PRC16_97_DEA_NAME	"Vel_data_level_precip_16_97"
#define VEL_DATA_LVL_PRC16_194_DEA_NAME	"Vel_data_level_precip_16_194"
#define VEL_DATA_LVL_PRC8_97_DEA_NAME	"Vel_data_level_precip_8_97"
#define VEL_DATA_LVL_PRC8_194_DEA_NAME	"Vel_data_level_precip_8_194"
#define VEL_DATA_LVL_CLR16_97_DEA_NAME	"Vel_data_level_clear_16_97"
#define VEL_DATA_LVL_CLR16_194_DEA_NAME	"Vel_data_level_clear_16_194"
#define VEL_DATA_LVL_CLR8_97_DEA_NAME	"Vel_data_level_clear_8_97"
#define VEL_DATA_LVL_CLR8_194_DEA_NAME	"Vel_data_level_clear_8_194"

/**  This structure is obsolete   **/
typedef	struct
{
	int 		num_levels;		/*  Number of levels used in this  table */
	unsigned short	code [NUM_REFLECTIVITY_DATA_LEVELS];
					/* Array holding the reflectivity  *
					 * data cutoff for each data level *
					 * code for each grid box of the   *
					 * Radar Coded Message (-33.0 -    *
					 * 95.0 dBZ).			   */

} reflect_data_level_t;


/**  This structure is obsolete		**/
typedef	struct {

	int	id;			/* Velocity threshold table code   */
	int	wx_mode;		/* Weather mode associated with	   *
					 * this table.			   */
	int	num_levels;		/* Number of data levels associated*
					 * with this table.		   */
	unsigned short code [NUM_VELOCITY_DATA_LEVELS];
					/* Array holding the lower velocity*
					 * data cutoff for each data level *
					 * code for base velocity products *
					 * (-244 - 244 kts).		   */

} velocity_data_level_t;

/*	The following structure defines the portion of legacy block	*
 *	PRODSEL which defines the VAD and RCM height levels.		*/

typedef	struct {

	int	vad [MAX_VAD_HEIGHTS];	/* VAD_AHTS */
					/* Array holding up to 30 levels  *
					 * for which VAD winds will be    *
					 * generated.			  */
	int	rcm [MAX_RCM_HEIGHTS];	/* VAD_RCM_HGTS */
					/* Array holding up to 19 levels  *
					 * for RCM height selection.  The *
					 * levels must also be defined	  *
					 * for VAD product.		  */

} vad_rcm_heights_t;

/**   Declare a C++ wrapper class that statically contains a pointer to the meta
      data for this structure.  The C++ wrapper class allows the IOCProperty API
      to be implemented automatically for this structure
**/


/**  This structure is obsolete   **/
typedef	struct {

	unsigned short	code [NUM_STP_DATA_LEVELS];
					/* Array containing data code info *
					 * for the STP and USP products.   *
					 * The permissible range is 0.1 to *
					 * 25.4 inches in multiples of 0.1 */

} stp_data_level_t;

/**  This structure is obsolete   **/
typedef	struct {

	unsigned short	code [NUM_OHP_THP_DATA_LEVELS];
					/* Array containing data code info *
					 * for the OHP, THP, USP products. *
					 * The permissible range is 0.05   *
					 * to 12.70 inches in multiples of *
					 * 0.05				   */
} ohp_thp_data_level_t;

#endif
