/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/03/08 19:00:30 $
 * $Id: orpgsite.h,v 1.22 2007/03/08 19:00:30 steves Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */

/**
	@api ORPGSITE
	@desc API used to access Site and Redundant adaptation data.
**/

#ifndef ORPGSITE_H
#define ORPGSITE_H

#include <orpgerr.h>
#include <orpgadpt.h>
#include <siteadp.h>
#include <mlos_info.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**  Macros to access dea values   */
#define ORPGSITE_DEA_RDA_LATITUDE	"site_info.rda_lat"
#define ORPGSITE_DEA_RDA_LONGITUDE	"site_info.rda_lon"
#define ORPGSITE_DEA_RDA_ELEVATION	"site_info.rda_elev"
#define ORPGSITE_DEA_RPG_ID		"site_info.rpg_id"
#define ORPGSITE_DEA_WX_MODE		"site_info.wx_mode"
#define ORPGSITE_DEA_DEF_MODE_A_VCP	"site_info.def_mode_A_vcp"
#define ORPGSITE_DEA_DEF_MODE_B_VCP	"site_info.def_mode_B_vcp"
#define ORPGSITE_DEA_HAS_MLOS		"site_info.has_mlos"
#define ORPGSITE_DEA_HAS_RMS		"site_info.has_rms"
#define ORPGSITE_DEA_REDUNDANT_TYPE	"Redundant_info.redundant_type"
#define ORPGSITE_DEA_CHANNEL_NO		"Redundant_info.channel_number"
#define ORPGSITE_DEA_HAS_BDDS		"site_info.has_bdds"
#define ORPGSITE_DEA_HAS_ARCHIVE_III	"site_info.has_archive_III"
#define ORPGSITE_DEA_IS_ORDA	        "site_info.is_orda"
#define ORPGSITE_DEA_PRODUCT_CODE	"site_info.product_code"
#define ORPGSITE_DEA_RPG_NAME		"site_info.rpg_name"
#define ORPGSITE_DEA_MLOS_NUM_STA 	"mlos_info.no_of_mlos_stations"
#define ORPGSITE_DEA_MLOS_STA_TYPE 	"mlos_info.station_type"


/**  Site property names   */
#define ORPGSITE_RDA_LATITUDE	"rda_lat"
#define ORPGSITE_RDA_LONGITUDE  "rda_lon"
#define ORPGSITE_RDA_ELEVATION	"rda_elev"
#define ORPGSITE_RPG_ID	   	"rpg_id"
#define ORPGSITE_WX_MODE	"wx_mode"
#define ORPGSITE_DEF_MODE_A_VCP "def_mode_A_vcp"
#define ORPGSITE_DEF_MODE_B_VCP "def_mode_B_vcp"
#define ORPGSITE_HAS_MLOS	"has_mlos"
#define ORPGSITE_HAS_RMS	"has_rms"
#define ORPGSITE_REDUNDANT_TYPE "redundant_type"
#define ORPGSITE_CHANNEL_NO	"channel_number"
#define ORPGSITE_HAS_BDDS	"has_bdds"
#define ORPGSITE_HAS_ARCHIVE_III "has_archive_III"
#define ORPGSITE_IS_ORDA         "is_orda"
#define ORPGSITE_PRODUCT_CODE   "product_code"
#define ORPGSITE_RPG_NAME	"rpg_name"

/**  Enumerated value for the redundant_type property  */
typedef enum { ORPGSITE_NO_REDUNDANCY=0, ORPGSITE_FAA_REDUNDANT=1, ORPGSITE_NWS_REDUNDANT=2 } orpgsite_redundant_type_t;

/**  Enumerated values for the wx_mode property  **/
typedef enum { ORPGSITE_CLEAR_AIR_MODE=1, ORPGSITE_PRECIP_MODE=2 } orpgsite_wx_mode_t;


/**  Causes ORPGSITE_log_last_error and ORPGSITE_last_error_text to report any low level details associated with the error */
#define ORPGSITE_REPORT_DETAILS		0x0001

/**  Causes ORPGSITE_log_last_error	and ORPGSITE_last_error_text to clear the last error message after their work is done */
#define ORPGSITE_CLEAR_ERROR		0x0002

/** Read in MLOS Data into Structure mlos_info */
int ORPGSITE_get_mlos_data( mlos_info_t *mlos_info );

/**  Read in Site Data into Structure Site */
int  ORPGSITE_get_site_data( Siteadp_adpt_t *site );

/**  Read in Redundant Data into Structure red_info */
int  ORPGSITE_get_redundant_data( Redundant_info_t *red_info );

/**  Force AscII site configuration file to be accessed */
void ORPGSITE_prefer_ascii();

/**  Get an integer site or redundant property value
     @param(in) prop_name - Get an integer site property
     @returns - an integer property value
     @side_effects - Exception information will be set
		     if an error occurs
*/
int ORPGSITE_get_int_prop(const char* prop_name);

/**  Get a floating point site or redundant property value
     @param(in) prop_name - Get an integer site property
     @returns - a floating point property value
     @side_effects - Exception information will be set
		     if an error occurs
*/
float ORPGSITE_get_float_prop(const char* prop_name);

/** Get a string site or redundant property value
     @param(in) prop_name - Get an integer site property
     @param(out)  buf - buffer to hold the output string property
     @param(in) buf_len - length of buffer
     @returns - a string property value if successful, NULL otherwise.
*/
char*  ORPGSITE_get_string_prop(const char* prop_name, char* buf, int buf_len);

/**  Log the last error that occurred using this API
     @param(in) le_code - LE information code
     @param(in) flags - flags to control error reporting (ORPGSITE_REPORT_DETAILS) and to
			clear the existing error after logging (ORPGSITE_CLEAR_ERROR)
     @returns - TRUE if an error was logged, FALSE otherwise.
**/

/**  Return the text associated with the last error message.
     @param(out) error_buf - buffer to hold the output error text
     @param(in) error_buf_len - length of error_buf
     @returns - TRUE if any error text was returned, FALSE otherwise.
*/

/**  Return TRUE if an error has occurred, and it has not be cleared by a
	ORPGSITE_clear_error or ORPGSITE_log_last_error call
*/

/**  Clear any error information for the ORPGSITE api  */
void ORPGSITE_clear_error();
int ORPGSITE_error_occurred();
int ORPGSITE_log_last_error (int le_code, int flags);
void ORPGSITE_update ();
void ORPGSITE_load_site_dea_files ();

#ifdef __cplusplus
}
#endif

#endif /* #ifndef ORPGSITE_H DO NOT REMOVE! */
