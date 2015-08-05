/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/08/19 21:05:52 $
 * $Id: orpgvst.c,v 1.20 2014/08/19 21:05:52 steves Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  orpgvst.c						*
 *									*
 *	Description:  This module contains a collection of functions	*
 *		      to manipulate Volume Status info message in 	*
 *		      the gen_stat_msg linear buffer.			*
 *									*
 ************************************************************************/

#include <orpg.h>
#include <orpgvst.h>
#include <orpgevt.h>

/*	Static variables						*/

static	Vol_stat_gsm_t	*Scan_info                   = NULL;
static	int		Init_vol_status_flag         = 0;
static	int		Vol_io_status                = 0;
static	int		ORPGVST_status_registered    = 0;
static	time_t		VST_status_update_time       = 0;

/************************************************************************
 *									*
 *	Description: This function returns the status of the last	*
 *		     Volume Status msg I/O operation.			*
 *									*
 ************************************************************************/
int ORPGVST_io_status(){

	return (Vol_io_status);

/* End of ORPGVST_io_status() */
}

/************************************************************************
 *									*
 *	Description: This function returns 0 if the Volume Status msg	*
 *		     needs to be updated, otherwise 1.			*
 *									*
 ************************************************************************/
int ORPGVST_status_update_flag(){

	return (Init_vol_status_flag);

/* End of ORPGVST_status_update_flag() */
}

/************************************************************************
 *									*
 *	Description: This function returns the time (in julian seconds)	*
 *	when the Volume Status message was last updated.		*
 *									*
 ************************************************************************/
time_t ORPGVST_status_update_time(){

	return (VST_status_update_time);

/* End of ORPGVST_status_update_time() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following routine sets the Volume Status       *
 *                   msg init flag whenever Volume Status message       *
 *                   LB notification is received.                       *
 *                                                                      *
 *      Return:      NONE                                               *
 *                                                                      *
 ************************************************************************/
void ORPGVST_lb_notify_callback( int fd, LB_id_t msgid, int msginfo, 
                                 void *arg ){
 
        Init_vol_status_flag = 0;

/* End of ORPGVST_lb_notify_callback() */
}

/************************************************************************
 *									*
 *	Description: The following routine reads the Volume Status msg	*
 *		     (VOL_STAT_GSM_ID) from the general status message	*
 *		     lb (ORPGDAT_GEN_STAT_MSG).				*
 *									*
 *	Return:      A pointer to the Volume Status Data or NULL on	*
 *		     error.						*
 *									*
 ************************************************************************/
char*  ORPGVST_read ( char* scan_info ){

	int	status;

/*      Check to see if Volume Status message LB notification has been  *
 *      registered.  If not, register it.                               */

        if (!ORPGVST_status_registered) {

            if( ((status = ORPGDA_write_permission( ORPGDAT_GSM_DATA )) < 0)
                                    ||
                ((status = ORPGDA_UN_register( ORPGDAT_GSM_DATA,
                                               VOL_STAT_GSM_ID, 
                                               ORPGVST_lb_notify_callback)) < 0) ){

                LE_send_msg (GL_CONFIG,
                        "ORPGVST: ORPGDA_UN_register failed (ret %d)\n", status);
                return (NULL);

            }

            ORPGVST_status_registered = 1;

        }

	Init_vol_status_flag = 1;

/*	If the passed argument is not NULL, set Scan_info.  Otherwise	*
        allocate Scan_info. 						*/
        if( scan_info != NULL ){

           Scan_info = (Vol_stat_gsm_t *) scan_info;
	   status = ORPGDA_read( ORPGDAT_GSM_DATA, Scan_info,
			         sizeof (Vol_stat_gsm_t), VOL_STAT_GSM_ID );

        }
        else{

           if( Scan_info == NULL )
	      status = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &Scan_info,
	   		            LB_ALLOC_BUF, VOL_STAT_GSM_ID );

           else{

              LE_send_msg( GL_ERROR, "Potential Memory Leak in ORPGVST_read()\n" );
	      status = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) Scan_info,
	   		            sizeof(Vol_stat_gsm_t), VOL_STAT_GSM_ID );

           }

        }

	Vol_io_status = status;

	if (status < 0) {

	    Init_vol_status_flag = 0;
	    LE_send_msg (GL_INFO, "ORPGDA_read (VOL_STAT_GSM_ID): %d\n", status);
            Scan_info = NULL;

	}
        else 
	    VST_status_update_time = time (NULL);

	return( (char *) Scan_info );

/* End of ORPGVST_read () */
}

/************************************************************************
 *									*
 *	Description: The following function returns the volume sequence	*
 *		     number.						*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
unsigned long ORPGVST_get_volume_number (){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (unsigned long) ORPGVST_DATA_NOT_FOUND );

	}

	return Scan_info->volume_number;

/* End of ORPGVST_get_volume_number() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following function returns the volume scan     *
 *                   number.                                            *
 *                                                                      *
 *      Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.    *
 *                                                                      *
 ************************************************************************/
int ORPGVST_get_volume_scan(){

        if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );

        }

        return Scan_info->volume_scan;

/* End of ORPGVST_get_volume_scan() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the current volume	*
 *		     scan time (milliseconds past midnight).		*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
unsigned long ORPGVST_get_volume_time(){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (unsigned long) ORPGVST_DATA_NOT_FOUND );

	}

	return Scan_info->cv_time;

/* End of ORPGVST_get_volume_time() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the value of the	*
 *		     initial volume flag.				*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
int ORPGVST_get_volume_flag(){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
	     
	}

	return (int) Scan_info->initial_vol;

/* End of ORPGVST_get_volume_flag() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the expected	*
 *		     volume duration (seconds).				*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
int ORPGVST_get_volume_duration(){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
	     
	}

	return (int) Scan_info->expected_vol_dur;

/* End of ORPGVST_get_volume_duration() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the current volume	*
 *		     scan julian date.					*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
int ORPGVST_get_volume_date (){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
	     
	}

	return (int) Scan_info->cv_julian_date;

/* End of ORPGVST_get_volume_date() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the previous	*
 *		     volume status.					*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *		     On success, ORPGVST_ABORTED or ORPGVST_SUCCESS	*
 *		     are returned.					*
 *									*
 ************************************************************************/
int ORPGVST_get_previous_status (){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
	     
	}

	return (int) Scan_info->pv_status;

/* End of ORPGVST_get_previous_status() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the current mode	*
 *		     of operation.					*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *		     On success, ORPGVST_PRECIPITATION_MODE, or		*
 *		     ORPGVST_CLEAR_AIR_MODE is returned.		*
 *									*
 ************************************************************************/
int ORPGVST_get_mode(){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );

	}

	return (int) Scan_info->mode_operation;

/* End of ORPGVST_get_mode() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the volume coverage	*
 *		     pattern (VCP) in use.				*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
int ORPGVST_get_vcp(){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
	     
	}

	return (int) Scan_info->vol_cov_patt;

/* End of ORPGVST_get_vcp() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following function returns the volume coverage *
 *                   pattern (VCP) index into the VCP definition table  *
 *                   of the VCP currently in use.                       *
 *                                                                      *
 *      Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.    *
 *                                                                      *
 ************************************************************************/
int ORPGVST_get_vcp_id(){

        if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
             
        }

        return (int) Scan_info->rpgvcpid;

/* End of ORPGVST_get_vcp_id() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the number of	*
 *		     elevation cuts in the current VCP.			*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
int ORPGVST_get_number_elevations(){

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
	     
	}

	return (int) Scan_info->num_elev_cuts;

/* End of ORPGVST_get_number_elevations() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the	elevation	*
 *		     angle (deg*10) for the specified elev index. 	*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 *	Note:	     The "indx" is the RDA elevation index - 1.		*
 *									*
 ************************************************************************/
int ORPGVST_get_elevation( int indx ){
 
	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );

	}

	if ((indx < 0) ||
	    (indx >= Scan_info->num_elev_cuts)) {

	    return (int) ORPGVST_DATA_NOT_FOUND;

	} else {

	    return (int) Scan_info->elevations [indx];

	}

/* End of ORPGVST_get_elevation() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the	RPG elevation	*
 *		     index associated with the corresponding elev	*
 *		     index.						*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 *	Note:        The "indx" is the RDA elevation index - 1.		*
 *									*
 ************************************************************************/
int ORPGVST_get_index( int indx ){ 

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );

	}

	if ((indx < 0) ||
	    (indx >= Scan_info->num_elev_cuts)) {

	    return (int) ORPGVST_DATA_NOT_FOUND;

	} else {

	    return (int) Scan_info->elev_index [indx];

	}

/* End of ORPGVST_get_index() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the	RDA elevation	*
 *		     index associated with the corresponding indx.      *
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 *	Note:        This is a worthless function and should be 	*
 *		     deprecated.					*
 *									*
 ************************************************************************/
int ORPGVST_get_rda_index( int indx ){

	int status, i;

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
	     
	}

        status = -1;
        for (i = 0; (i < Scan_info->num_elev_cuts); i++)
        {
            if (Scan_info->elev_index[i] == indx)
            {
                status = i;
                break;
            }
        }
	if (status == -1)
           status = ORPGVST_DATA_NOT_FOUND;
	return (status);

/* End of  ORPGVST_get_rda_index() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the	current VCP  	*
 *		     table in the user supplied buffer.                 *
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
int ORPGVST_get_current_vcp( Vcp_struct *vcp ){

        if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );
             
        }

        if( vcp == NULL ) 
            return (int) ORPGVST_DATA_NOT_FOUND;

        memcpy( vcp, &Scan_info->current_vcp_table, sizeof(Vcp_struct) );

        return(0);

/* End of ORPGVST_get_current_vcp() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following function returns the expected super  *
 *                   res bit map.					*
 *                                                                      *
 *      Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.    *
 *                                                                      *
 ************************************************************************/
int ORPGVST_get_superres_bitmap( ){

        if (!Init_vol_status_flag) {

            if( ORPGVST_read( (char *) Scan_info ) == NULL )
                return( (int) ORPGVST_DATA_NOT_FOUND );

        }

        return( Scan_info->super_res_cuts );

/* End of ORPGVST_get_superres_bitmap() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following function returns the Dual Pol        *
 *                   expected value.		*
 *                                                                      *
 *      Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.    *
 *                                                                      *
 ************************************************************************/
int ORPGVST_get_dual_pol_expected(){

        if (!Init_vol_status_flag) {

            if( ORPGVST_read( (char *) Scan_info ) == NULL )
                return( (int) ORPGVST_DATA_NOT_FOUND );

        }

        return( Scan_info->dual_pol_expected );

/* End of ORPGVST_get_dual_pol_expected() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following function returns 1 (super res) or    *
 *                   0 (not super res) based on rpg elevation index.	*
 *                                                                      *
 *      Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.    *
 *                                                                      *
 *	Notes:	     The rpg elevation number begins with 1. The bit	*
 *		     map uses bit 0 to denote elevation 1.		*
 *									*
 ************************************************************************/
int ORPGVST_is_rpg_elev_superres( int rpg_elev_num ){

	int bit;

        if (!Init_vol_status_flag) {

            if( ORPGVST_read( (char *) Scan_info ) == NULL )
                return( (int) ORPGVST_DATA_NOT_FOUND );

        }

        bit = 1 << (rpg_elev_num - 1);
        if( Scan_info->super_res_cuts & bit )
           return 1;

        return( 0 );

/* End of ORPGVST_is_rpg_elev_superres() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following function returns the VCP Supplemental*
 *                   bitmap.            			 	*
 *                                                                      *
 *      Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.    *
 *                                                                      *
 ************************************************************************/
int ORPGVST_get_vcp_supplemental_data(){

        if (!Init_vol_status_flag) {

            if( ORPGVST_read( (char *) Scan_info ) == NULL )
                return( (int) ORPGVST_DATA_NOT_FOUND );

        }

        return( Scan_info->vcp_supp_data );

/* End of ORPGVST_get_vcp_supplemental_data() */
}

/************************************************************************
 *									*
 *	Description: The following function returns the	number of SAILS *
 *		     cuts in the current vcp.                    	*
 *									*
 *	Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.	*
 *									*
 ************************************************************************/
int ORPGVST_get_n_sails_cuts(){ 

	if (!Init_vol_status_flag) {

	    if( ORPGVST_read( (char *) Scan_info ) == NULL )
		return( (int) ORPGVST_DATA_NOT_FOUND );

	}

	return (int) Scan_info->n_sails_cuts;

/* End of ORPGVST_get_n_sails_cuts() */
}

/************************************************************************
 *                                                                      *
 *      Description: The following function returns the SAILS cut       *
 *                   sequence number for the cut, or 0 if not a SAILS	*
 *                   cut. 						*
 *                                                                      *
 *      Return:      On failure, ORPGVST_DATA_NOT_FOUND is returned.    *
 *                                                                      *
 *      Note:        The "indx" is the RDA elevation index - 1.         *
 *                                                                      *
 ************************************************************************/
int ORPGVST_get_sails_cut_seq( int indx ){
 
        if (!Init_vol_status_flag) {

            if( ORPGVST_read( (char *) Scan_info ) == NULL )
                return( (int) ORPGVST_DATA_NOT_FOUND );

        }

        if ((indx < 0) ||
            (indx >= Scan_info->num_elev_cuts)) {

            return (int) ORPGVST_DATA_NOT_FOUND;

        } else {

            return (int) Scan_info->sails_cut_seq [indx];

        }

/* End of ORPGVST_get_sails_cut_seq() */
}

