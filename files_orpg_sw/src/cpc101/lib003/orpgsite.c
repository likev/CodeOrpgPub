
/******************************************************************

    The ORPGSITE module.
 
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/03/08 20:18:01 $
 * $Id: orpgsite.c,v 1.13 2007/03/08 20:18:01 jing Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <orpgsite.h>
#include <orpg.h>
#include <infr.h>
#include <siteadp.h>
#include <mlos_info.h>


#define ORPGSITE_ERR_VALUE -99.f
static int Need_update = 1;
static int Use_ascii = 0;

static char *Get_de_name (const char *data_name);
static void Read_site_data ();
static int Get_enum_value (char *de_name, double *value);

/***********************************************************************

    Description:
        Returns site information in structure "site".

    Inputs:
        site = pointer to Siteadp_adapt_t structure.

    Returns:
        Non-negative integer on success, negative integer on error.

***********************************************************************/
int ORPGSITE_get_mlos_data( mlos_info_t *mlos_info ){

    char *ac_ds_name = NULL;
    double values[4] = {0.0};
    int i, retval;

    /* First check that the returned structure address is valid. */
    if( mlos_info == NULL )
       return -1;

    /* Set the DEAU LB name. */
    if( (ac_ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA )) == NULL ){

        LE_send_msg( GL_ERROR, "ORPGSITE: ORPGDA_lbname (%d) failed", 
                     ORPGDAT_ADAPT_DATA );
	return -1;
    }

    DEAU_LB_name( ac_ds_name );

    /* Get all the data elements. */
    if( (retval = DEAU_get_values( ORPGSITE_DEA_MLOS_NUM_STA, &values[0], 1 )) < 0 )
       return retval;

    mlos_info->no_of_mlos_stations = (int) values[0];

    if( (retval = DEAU_get_values( ORPGSITE_DEA_MLOS_STA_TYPE, &values[0], 4 )) < 0 )
       return -1;

    for( i = 0; i < mlos_info->no_of_mlos_stations; i++ )
       mlos_info->station_type[i] = (mlos_station_type_t) values[i];

    return 0;

}

/***********************************************************************

    Description:
        Returns site information in structure "site".

    Inputs:
        site = pointer to Siteadp_adapt_t structure.

    Returns:
        Non-negative integer on success, negative integer on error.

***********************************************************************/
int ORPGSITE_get_redundant_data( Redundant_info_t *red_info ){

    char *ac_ds_name = NULL;
    double value = 0.0;
    int retval;

    /* First check that the returned structure address is valid. */
    if( red_info == NULL )
       return -1;

    /* Set the DEAU LB name. */
    if( (ac_ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA )) == NULL ){

        LE_send_msg( GL_ERROR, "ORPGSITE: ORPGDA_lbname (%d) failed", 
                     ORPGDAT_ADAPT_DATA );
	return -1;
    }

    DEAU_LB_name( ac_ds_name );

    /* Get all the data elements. */
    if( (retval = DEAU_get_values( ORPGSITE_DEA_REDUNDANT_TYPE, &value, 1 )) < 0 )
       return retval;

    red_info->redundant_type = (int) value;

    if( (retval = DEAU_get_values( ORPGSITE_DEA_CHANNEL_NO, &value, 1 )) < 0 )
       return -1;

    red_info->channel_number = (int) value;

    return 0;

}

/***********************************************************************

    Description:
        Returns site information in structure "site".

    Inputs:
        site = pointer to Siteadp_adapt_t structure.

    Returns:
        Non-negative integer on success, negative integer on error.

***********************************************************************/
int ORPGSITE_get_site_data( Siteadp_adpt_t *site ){

    char *str = NULL, *ac_ds_name = NULL;
    double value = 0.0;

    /* First check that the returned structure address is valid. */
    if( site == NULL )
       return -1;

    /* Set the DEAU LB name. */
    if( (ac_ds_name = ORPGDA_lbname( ORPGDAT_ADAPT_DATA )) == NULL ){

        LE_send_msg( GL_ERROR, "ORPGSITE: ORPGDA_lbname (%d) failed", 
                     ORPGDAT_ADAPT_DATA );
	return -1;
    }

    DEAU_LB_name( ac_ds_name );

    /* Get all the data elements. */
    if( DEAU_get_values( ORPGSITE_DEA_RDA_LATITUDE, &value, 1 ) < 0 )
       return -1;

    site->rda_lat = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_RDA_LONGITUDE, &value, 1 ) < 0 )
       return -1;

    site->rda_lon = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_RDA_ELEVATION, &value, 1 ) < 0 )
       return -1;

    site->rda_elev = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_RPG_ID, &value, 1 ) < 0 )
       return -1;

    site->rpg_id = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_WX_MODE, &value, 1 ) < 0 )
       return -1;

    site->wx_mode = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_DEF_MODE_A_VCP, &value, 1 ) < 0 )
       return -1;

    site->def_mode_A_vcp = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_DEF_MODE_B_VCP, &value, 1 ) < 0 )
       return -1;

    site->def_mode_B_vcp = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_HAS_MLOS, &value, 1 ) < 0 )
       return -1;

    site->has_mlos = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_HAS_RMS, &value, 1 ) < 0 )
       return -1;

    site->has_rms = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_HAS_BDDS, &value, 1 ) < 0 )
       return -1;

    site->has_bdds = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_HAS_ARCHIVE_III, &value, 1 ) < 0 )
       return -1;

    site->has_archive_III = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_IS_ORDA, &value, 1 ) < 0 )
       return -1;

    site->is_orda = (int) value;

    if( DEAU_get_values( ORPGSITE_DEA_PRODUCT_CODE, &value, 1 ) < 0 )
       return -1;

    site->product_code = (int) value;

    if( DEAU_get_string_values( ORPGSITE_DEA_RPG_NAME, &str ) < 0 )
       return -1;

    strcpy( site->rpg_name, str );

    return 0;

}

/***********************************************************************

    Returns the integer value of site data "data_name". -99 is returned
    in case of error. The first value is returned in case there are 
    multiple values.

***********************************************************************/

int ORPGSITE_get_int_prop (const char *data_name) {
    return ((int)ORPGSITE_get_float_prop (data_name));
}

/***********************************************************************

    Returns the float value of site data "data_name". -99 is returned
    in case of error. The first value is returned in case there are 
    multiple values.

***********************************************************************/

float ORPGSITE_get_float_prop (const char *data_name) {
    double value;
    char *de_name;
    int ret;

    if (Need_update)
	Read_site_data ();

    de_name = Get_de_name ((char *)data_name);
    if (de_name == NULL)
	return (ORPGSITE_ERR_VALUE);
    ret = DEAU_get_values (de_name, &value, 1);
    if (ret < 0) {
	if (Use_ascii && ret == DEAU_BAD_NUMERICAL) {
	    if (Get_enum_value (de_name, &value) < 0)
		return (ORPGSITE_ERR_VALUE);
	}
	else {
	    LE_send_msg (GL_ERROR, 
		"ORPGSITE: DEAU_get_values (%s) failed (%d)\n", de_name, ret);
	    return (ORPGSITE_ERR_VALUE);
	}
    }
    return ((float)value);
}

/***********************************************************************

    Returns the pointer to the string value of site data "data_name". 
    NULL is returned in case of error. The string value is copied to
    buffer "buf" of length "buf_len". The string value is truncated in
    case that "buf" is too small. The first value is returned in case 
    there are multiple values.

***********************************************************************/

char *ORPGSITE_get_string_prop (const char *data_name, 
						char *buf, int buf_len) {
    char *de_name, *value;
    int ret;

    if (Need_update)
	Read_site_data ();

    de_name = Get_de_name ((char *)data_name);
    if (de_name == NULL)
	return (0);
    ret = DEAU_get_string_values (de_name, &value);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGSITE: DEAU_get_values (%s) failed (%d)\n", de_name, ret);
	return (NULL);
    }
    strncpy (buf, value, buf_len);
    buf[buf_len - 1] = '\0';
    return (buf);
}

/***********************************************************************

    Do nothing - This is for compatibility.

***********************************************************************/

int ORPGSITE_log_last_error (int le_code, int flags) {
    return (0);
}

/***********************************************************************

    Do nothing - This is for compatibility.

***********************************************************************/

int ORPGSITE_last_error_text (char *error_buf, int error_buf_len, int flags) {
    error_buf[0] = '\0';
    return (0);
}

/***********************************************************************

    Do nothing - This is for compatibility.

***********************************************************************/

int ORPGSITE_error_occurred () {
    return (0);
}

/***********************************************************************

    Do nothing - This is for compatibility.

***********************************************************************/

void ORPGSITE_clear_error () {
}

void ORPGSITE_prefer_ascii () {
}

/***********************************************************************

    Loads DEA from site DEA files named "site_info.dea" and
    "dea/site_info.gen" in the config directory if they exist. This
    causes the site data in the DEA files be used instead of that in
    the database. If "site_info.dea" is not found, this function will
    have no effect.

***********************************************************************/

#define ORPGSITE_NAME_SIZE 256

void ORPGSITE_load_site_dea_files () {
    char si[ORPGSITE_NAME_SIZE], sig[ORPGSITE_NAME_SIZE];
    int ret;

    ret = MISC_get_cfg_dir (si, ORPGSITE_NAME_SIZE - 32);
    if (ret < 0)
	return;
    strcpy (sig, si);
    strcat (si, "/site_info.dea");
    if (DEAU_use_attribute_file (si, 1) < 0) {
	LE_send_msg (GL_INFO, "ORPGSITE: DEA file (%s) not found", si);
	return;
    }
    strcat (sig, "/dea/site_info.gen");
    ret = DEAU_use_attribute_file (sig, 1);

    Use_ascii = 1;
    Need_update = 0;
}

/***********************************************************************

    Sets Need_update so the site info will reread from the data base.

***********************************************************************/

void ORPGSITE_update () {
    Need_update = 1;
}

/***********************************************************************

    Returns the data element name for "data_name".

***********************************************************************/

typedef struct {
    char *call_name;
    char *alt_name;
    char *de_id;
} Data_name_t;

static char *Get_de_name (const char *data_name) {
    static const Data_name_t data_names[] = {
	{ORPGSITE_RDA_LATITUDE, "rda_lat", "site_info.rda_lat"},
	{ORPGSITE_RDA_LONGITUDE, "rda_lon", "site_info.rda_lon"},
	{ORPGSITE_RDA_ELEVATION, "rda_elev", "site_info.rda_elev"},
	{ORPGSITE_RPG_ID, "rpg_id", "site_info.rpg_id"},
	{ORPGSITE_WX_MODE, "wx_mode", "site_info.wx_mode"},
	{ORPGSITE_DEF_MODE_A_VCP, "def_mode_A_vcp",
					 "site_info.def_mode_A_vcp"},
	{ORPGSITE_DEF_MODE_B_VCP, "def_mode_B_vcp", 
					"site_info.def_mode_B_vcp"},
	{ORPGSITE_HAS_MLOS, "MLOS", "site_info.has_mlos"},
	{ORPGSITE_HAS_RMS, "RMS", "site_info.has_rms"},
	{ORPGSITE_REDUNDANT_TYPE, "Redundant", "Redundant_info.redundant_type"},
	{ORPGSITE_CHANNEL_NO, "channel_number", "Redundant_info.channel_number"},
	{ORPGSITE_HAS_BDDS, "BDDS", "site_info.has_bdds"},
	{ORPGSITE_HAS_ARCHIVE_III, "Archive III", "site_info.has_archive_III"},
	{ORPGSITE_PRODUCT_CODE, "product_code", "site_info.product_code"},
	{ORPGSITE_RPG_NAME, "rpg_name", "site_info.rpg_name"}
    };
    static int n_data_names = sizeof (data_names) / sizeof (Data_name_t);
    int ind;

    for (ind = 0; ind < n_data_names; ind++) {
	if (strcmp (data_name, data_names[ind].call_name) == 0 ||
	    strcmp (data_name, data_names[ind].alt_name) == 0)
	    return (data_names[ind].de_id);
    }
    LE_send_msg (GL_ERROR, 
		"ORPGSITE: Site data name (%s) not found\n", data_name);
    return (NULL);
}

/***********************************************************************

    Reads site data from the DEA database.

***********************************************************************/

static void Read_site_data () {
    char *ac_ds_name;

    ac_ds_name = ORPGDA_lbname (ORPGDAT_ADAPT_DATA);
    if (ac_ds_name == NULL) {
	LE_send_msg (GL_ERROR, 
		"ORPGSITE: ORPGDA_lbname (%d) failed", ORPGDAT_ADAPT_DATA);
	exit (1);
    }
    DEAU_LB_name (ac_ds_name);

/* Prevent caching of DEA data on MSCF. This is to fix R1577. 
   This section of code may eventually need to be removed. */
/*
    ret = DEAU_read_listed_attrs ("site_info.*");
    if (ret < 0) {
	LE_send_msg (GL_ERROR, 
		"ORPGSITE: DEAU_read_listed_attrs failed (%d)", ret);
	exit (1);
    }
*/

    Need_update = 0;
}

/***********************************************************************

    This function is for temporarily support mscf where dea/site_info.gen
    is not available. mscf will be updated later to not depend on any 
    local adaptation data. Then this function will not be needed. This
    function reads "de_name" as a string and converts it to its enum 
    value. Returns 0 on success or -1 on failure.

***********************************************************************/

static int Get_enum_value (char *de_name, double *value) {
    int ret;
    char *p;

    *value = -1.;
    ret = DEAU_get_string_values (de_name, &p);
    if (ret <= 0) {
	LE_send_msg (GL_ERROR, 
	"ORPGSITE: DEAU_get_string_values (%s) failed (%d)\n", de_name, ret);
	return (-1);
    }
    if (strstr (de_name, "has_mlos") != NULL ||
	strstr (de_name, "has_rms") != NULL ||
	strstr (de_name, "has_bdds") != NULL ||
	strstr (de_name, "has_archive_III") != NULL) {
	if (strcmp (p, "No") == 0)
	    *value = 0.;
	else if (strcmp (p, "Yes") == 0)
	    *value = 1.;
    }
    else if (strstr (de_name, "redundant_type") != NULL) {
	if (strcmp (p, "No Redundancy") == 0)
	    *value = 0.;
	else if (strcmp (p, "FAA Redundant") == 0)
	    *value = 1.;
	else if (strcmp (p, "NWS Redundant") == 0)
	    *value = 2.;
    }
    else if (strstr (de_name, "channel_number") != NULL) {
	if (strcmp (p, "Channel 1") == 0)
	    *value = 1.;
	else if (strcmp (p, "Channel 2") == 0)
	    *value = 2.;
    }
    else if (strstr (de_name, "wx_mode") != NULL) {
	if (strcmp (p, "Clear Air") == 0)
	    *value = 1.;
	else if (strcmp (p, "Precipitation") == 0)
	    *value = 2.;
    }
    if (*value < 0.) {
	LE_send_msg (GL_ERROR, 
	    "ORPGSITE: Unexpected value (%s) for (%d)\n", p, de_name);
	return (-1);
    }
    return (0);
}


void ORPGSITE_init() {return;}



