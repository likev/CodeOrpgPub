 /*
  * RCS info
  * $Author: steves $
  * $Locker:  $
  * $Date: 2013/06/05 14:19:28 $
  * $Id: hci_vcp_data_functions.c,v 1.34 2013/06/05 14:19:28 steves Exp $
  * $Revision: 1.34 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: hci_vcp_functions.c					*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used by the HCI to interface with VCP data.			*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_vcp_data.h>

/*	Local global variables.						*/

static	Vol_stat_gsm_t  Vcp_info; /* Buffer for VCP data */
static	Vol_stat_gsm_t  Vcp_tmp;  /* Temporary buffer for VCP data */
static	int	Init_flag = 0;    /* Initialization flag */
static	int	Lock_flag = 0;    /* Lock flag */

static	float	Prf_value [] = { /* PRF values for each PRF number */

	321.888,  446.428,  643.777,  857.143,
	1013.51,  1094.89,  1181.00,  1282.05

};

static	int	Unambiguous_range [][LAST_PRF] = {

	/* Unambiguous range for each PRF number and delta PRI */
	{460, 332, 230, 173, 146, 135, 125, 115},	/* Delta PRI 1 */
	{463, 334, 232, 174, 147, 136, 126, 116},	/* Delta PRI 2 */
	{466, 336, 233, 175, 148, 137, 127, 117},	/* Delta PRI 3 */
	{468, 338, 234, 176, 149, 138, 128, 118},	/* Delta PRI 4 */
	{471, 340, 236, 177, 150, 139, 129, 119}	/* Delta PRI 5 */

};

static	int	Unambiguous_range_sprt [][LAST_PRF] = {

	/* Unambiguous range for each PRF number and delta PRI.  For Staggered
	   PRT, there is no distinction for Delta PRI. */
	{261, 243, 224, 206, 187, 169, 150, 132},	/* Delta PRI 1 */
	{261, 243, 224, 206, 187, 169, 150, 132},	/* Delta PRI 2 */
	{261, 243, 224, 206, 187, 169, 150, 132},	/* Delta PRI 3 */
	{261, 243, 224, 206, 187, 169, 150, 132},	/* Delta PRI 4 */
	{261, 243, 224, 206, 187, 169, 150, 132}	/* Delta PRI 5 */

};

static	int	Vcp_registered = 0; /* LB notification register flag */
static	int	Vcp_io_status      = 0; /* Last read status */

static	void	hci_current_vcp_register (int fd, LB_id_t msg_id,
			int msg_info, void *arg);

/************************************************************************
 *	Description: This function returns the last read/write status.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: last read status					*
 ************************************************************************/
int hci_vcp_io_status(){

	return(Vcp_io_status);

/* End of hci_vcp_io_status() */
}

/************************************************************************
 *	Description: This function reads the current VCP data.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: read status (negative - error)				*
 ************************************************************************/
int hci_read_current_vcp_data (){

	int	status;
	
	if (!Vcp_registered) {

	    Vcp_registered = 1;

	    ORPGDA_write_permission( ORPGDAT_GSM_DATA );
	    status = ORPGDA_UN_register( ORPGDAT_GSM_DATA, VOL_STAT_GSM_ID,
			                 hci_current_vcp_register );

	    if (status != LB_SUCCESS) {

	        HCI_LE_error(
		"LIB: ORPGDA_UN_register (ITC_CD07_VCPINFO) failed (%d)", status );

	    }

	}

	status = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &Vcp_info,
			      sizeof (Vol_stat_gsm_t), VOL_STAT_GSM_ID );
	Vcp_io_status = status;
	if (status <= 0) {

	    HCI_LE_error( "ORPGDA_read(ORPGDAT_GSM_DATA) failed: %d",
		         status );

	    Init_flag = 0;

	} else {

	    Init_flag = 1;

	}

	return status;
}

/************************************************************************
 *	Description: This function writes the current VCP data.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: write status (negative - error)				*
 ************************************************************************/

int
hci_write_current_vcp_data (
)
{
	int   	status;
	int   	i;
        int     pass = 0;
	
	HCI_LE_status( "Updating current vcp info data" );

        while(1){

/*	Before we write the data, let's make sure we retain the		*
 *	current sequence number.  So lets temporarily save what		*
 *	we have and read the latest VCP info so we can get the		*
 *	latest sequence number.  Then, update the VCP portion of	*
 *	the structure and write it back.				*/

	    Vcp_tmp = Vcp_info;

	    status = hci_read_current_vcp_data ();
	    Vcp_io_status = status;

/* 	If the current VCP read is not the same VCP as last VCP read,	*
 *      ignore the write. 						*/

            if( status > 0 ){

                int vcp_c = Vcp_info.current_vcp_table.vcp_num;
                int vcp_p = Vcp_tmp.current_vcp_table.vcp_num;

                if( vcp_c != vcp_p ){ 

                   HCI_LE_error( "VCP Number Mismatch.  hci_write_current_vcp_data Failed" );
                   status = -1;

                }

            }

	    if( status <= 0 ) 
	        return status;

/*	Copy the changed VCP data to the just read structure and write	*
 *	it back.							*/

/*	For MPDA VCPs and SZ2 cuts, we are not allowed to modify any PRF info.	*/
            if( (abs(Vcp_tmp.current_vcp_table.vcp_num) < VCP_MIN_MPDA)
                                     ||
                (abs(Vcp_tmp.current_vcp_table.vcp_num) > VCP_MAX_MPDA) ){

                for( i = 0; i < Vcp_info.current_vcp_table.n_ele; i++ ){

                    Ele_attr *elv_info = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele[i][0];
                    Ele_attr *elv_tmp = (Ele_attr *) &Vcp_tmp.current_vcp_table.vcp_ele[i][0];

                    /* Transfer information for non-SZ2 cuts. */
                    if( elv_info->phase != VCP_PHASE_SZ2 )
                        memcpy( (void *) elv_info, (void *) elv_tmp, sizeof(Ele_attr));
                    
                    else {

                        /* Transfer information for SZ2 cuts if PRFs all the same. */
                        if( (elv_tmp->dop_prf_num_1 == elv_tmp->dop_prf_num_2)
                                                    &&
                            (elv_tmp->dop_prf_num_1 == elv_tmp->dop_prf_num_3) )
                            memcpy( (void *) elv_info, (void *) elv_tmp, sizeof(Ele_attr));

                    }

                }

	    }


/*	Transfer the Velocity Increment.				*/
            Vcp_info.current_vcp_table.vel_resolution = 
                        Vcp_tmp.current_vcp_table.vel_resolution;

/*	Transfer the SNR threshold data.				*/
            for( i = 0; i < Vcp_info.current_vcp_table.n_ele; i++ ){

               Ele_attr *elv_info = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele[i][0];
               Ele_attr *elv_tmp = (Ele_attr *) &Vcp_tmp.current_vcp_table.vcp_ele[i][0];

               elv_info->surv_thr_parm = elv_tmp->surv_thr_parm; 
               elv_info->vel_thrsh_parm = elv_tmp->vel_thrsh_parm; 
               elv_info->spw_thrsh_parm = elv_tmp->spw_thrsh_parm; 

            }

/* 	If during the time we read the data, modified it and are now 	*
 *	wanting to write it out the Volume Status has been updated, 	*
 *	read the Volume Status again . 					*/
            if( !Init_flag ){

                pass++;

/* 	This is more or less a sanity check .... should never happen 	*
 *	unless there is a coding error.                             	*/
                if( pass >= 2 ){

                   HCI_LE_error( "Something's Wrong!!! hci_write_current_vcp_data Failed" );
                   return(-1);

                }

                continue;

            }


	    status = ORPGRDA_send_cmd( COM4_DLOADVCP, HCI_VCP_INITIATED_RDA_CTRL_CMD, 0, 1, 0, 0, 0, (void *) &Vcp_info.current_vcp_table, sizeof( Vcp_struct ) ); 

	    Vcp_io_status = status;
	    if (status <= 0) {

		HCI_LE_error( "ORPGRDA_send_cmd() failed: %d", status );

	    }

	    return status;

	}

}

/************************************************************************
 *	Description: This function returns the VCP number currently	*
 *		     being used.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: VCP number						*
 ************************************************************************/

int
hci_current_vcp (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.vol_cov_patt;
}

/************************************************************************
 *	Description: This function returns the position in the VCP	*
 *		     table matching the current VCP.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: VCP id							*
 ************************************************************************/

int
hci_current_vcp_id (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.rpgvcpid;
}

/************************************************************************
 *	Description: This function returns the weather mode of the	*
 *		     current VCP.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: 1 - Clear Air; 2 - Precipitation			*
 ************************************************************************/

int
hci_current_vcp_wxmode (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.mode_operation;
}

/************************************************************************
 *	Description: This function returns the volume scan sequence of	*
 *		     the current VCP.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: volume sequence number					*
 ************************************************************************/

int
hci_current_vcp_seqnum (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.volume_number;
}

/************************************************************************
 *	Description: This function returns the velocity resolution of	*
 *		     the current VCP.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: velocity resolution code				*
 ************************************************************************/

int
hci_current_vcp_get_vel_resolution (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.current_vcp_table.vel_resolution;
}

/************************************************************************
 *	Description: This function sets the velocity resolution of	*
 *		     the current VCP.					*
 *									*
 *	Input:  num - velocity resolution code				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_set_vel_resolution (
int	num
)
{
	Vcp_info.current_vcp_table.vel_resolution = (short) num;
}

/************************************************************************
 *	Description: This function returns the volume coverage pattern	*
 *		     (VCP) type of the current VCP.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: VCP type						*
 ************************************************************************/

int
hci_current_vcp_type (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.current_vcp_table.type;
}

/************************************************************************
 *	Description: This function returns the volume coverage pattern	*
 *		     (VCP) number of the current VCP.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: VCP number						*
 ************************************************************************/

int
hci_current_vcp_num (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.current_vcp_table.vcp_num;
}

/************************************************************************
 *	Description: This function returns the number of elevation cuts	*
 *		     in the current VCP.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of elevation cuts				*
 ************************************************************************/

int
hci_current_vcp_num_elevations (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.current_vcp_table.n_ele;
}

/************************************************************************
 *	Description: This function returns the clutter map number	*
 *		     in the current VCP.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: clutter map number					*
 ************************************************************************/

int
hci_current_vcp_clutter_map_num (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.current_vcp_table.clutter_map_num;
}

/************************************************************************
 *	Description: This function returns the pulse width of the	*
 *		     current VCP.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pulse width						*
 ************************************************************************/

int
hci_current_vcp_pulse_width (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return 0;

	    }
	}

	return Vcp_info.current_vcp_table.pulse_width;
}

/************************************************************************
 *	Description: This function returns a pointer to the current	*
 *		     VCP data in the VCP table				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to VCP data					*
 ************************************************************************/

Vcp_struct
*hci_current_vcp_data_ptr (
)
{
	int	status;

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return (Vcp_struct *) NULL;

	    }
	}

	return &Vcp_info.current_vcp_table;
}

/************************************************************************
 *	Description: This function returns the elevation angle for a	*
 *		     cut in the current VCP.				*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: elevation angle (degrees)				*
 ************************************************************************/

float
hci_current_vcp_elevation_angle (
int	cut
)
{
	int	status, elev_angle_deg10;
	Ele_attr	*ele_attr;
	float	elev_angle_deg;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -99.0;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	elev_angle_deg = (float) ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                                      ele_attr->ele_angle );

        if( elev_angle_deg >= 0.0 )
           elev_angle_deg10 = elev_angle_deg*10 + 0.5;

        else
           elev_angle_deg10 = elev_angle_deg*10 - 0.5;

	return (float) elev_angle_deg10/10.0; 

}

/************************************************************************
 *	Description: This function returns the waveform type for a	*
 *		     cut in the current VCP.				*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: waveform type						*
 ************************************************************************/

int
hci_current_vcp_wave_type (
int	cut
)
{
	int	status;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -1;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	return (int) (ele_attr->wave_type);

}

/************************************************************************
 *      Description: This function returns the phase type for a         *
 *                   cut in the current VCP.                            *
 *                                                                      *
 *      Input:  cut - elevation cut number                              *
 *      Output: NONE                                                    *
 *      Return: phase type                                              *
 ************************************************************************/

int
hci_current_vcp_phase (
int     cut
)
{
        int     status;
        Ele_attr        *ele_attr;
        unsigned char   phase;

/*      Make sure the structure has been initialized.  If not, return   *
 *      with a negative number to indicate it was unsucessfull.         */

        if (!Init_flag) {

            status = hci_read_current_vcp_data ();

            if (status <= 0) {

                return -1;

            }
        }

        ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

        phase = ele_attr->phase;
        return (int) (phase);

}


/************************************************************************
 *      Description: This function returns the Dual-pol type for a cut  *
 *                   in the current VCP.                                *
 *                                                                      *
 *      Input:  cut - elevation cut number                              *
 *      Output: NONE                                                    *
 *      Return: Dual pol type                                           *
 ************************************************************************/

int
hci_current_vcp_dual_pol( int cut )
{
  int status;
  Ele_attr *ele_attr;

  /* Make sure the structure has been initialized.  If not, return
     with a negative number to indicate it was unsucessfull. */

  if( !Init_flag )
  {
    status = hci_read_current_vcp_data();
    if( status <= 0 ){ return -1; }
  }

  ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele[cut];

  return (int) ( ele_attr->super_res & VCP_DUAL_POL_ENABLED );
}

/************************************************************************
 *      Description: This function returns the super resolution type    *
 *                   for a cut in the current VCP.                      *
 *                                                                      *
 *      Input:  cut - elevation cut number                              *
 *      Output: NONE                                                    *
 *      Return: super resolution type                                   *
 ************************************************************************/

int
hci_current_vcp_super_res (
int     cut
)
{
        int     status;
        Ele_attr        *ele_attr;

/*      Make sure the structure has been initialized.  If not, return   *
 *      with a negative number to indicate it was unsucessfull.         */

        if (!Init_flag) {

            status = hci_read_current_vcp_data ();

            if (status <= 0) {

                return -1;

            }
        }

        ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

        return (int) (ele_attr->super_res & HCI_ELEV_SR_BITMASK);

}


/************************************************************************
 *	Description: This function returns the surveillance PRF number	*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: PRF number						*
 ************************************************************************/

int
hci_current_vcp_surv_prf_number (
int	cut
)
{
	int	status;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -1;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	return (int) ele_attr->surv_prf_num;

}

/************************************************************************
 *	Description: This function returns the surveillance pulse count	*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: pulse count						*
 ************************************************************************/

int
hci_current_vcp_surv_pulse_count (
int	cut
)
{
	int	status;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -1;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	return (int) ele_attr->surv_pulse_cnt;

}

/************************************************************************
 *	Description: This function returns the azimuth scan rate	*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: rate (deg/sec)						*
 ************************************************************************/

int
hci_current_vcp_azimuth_rate (
int	cut
)
{
	int	status;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -1;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	return (int) ele_attr->azi_rate;

}

/************************************************************************
 *	Description: This function returns the reflectivity noise	*
 *		     threshold for a cut in the current VCP.		*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: noise threshold (dB)					*
 ************************************************************************/

float
hci_current_vcp_get_ref_noise_threshold (
int	cut
)
{
	int	status;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -999.9;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	return (float) (ele_attr->surv_thr_parm/VCP_SNR_SCALE);

}

/************************************************************************
 *	Description: This function sets the reflectivity noise		*
 *		     threshold for a cut in the current VCP.		*
 *									*
 *	Input:  cut - elevation cut number				*
 *		num - noise threshold (dB)				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_set_ref_noise_threshold (
int	cut,
float	num
)
{
	Ele_attr	*ele_attr;

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	if (num >= 0) {

	    ele_attr->surv_thr_parm = (int) (num*VCP_SNR_SCALE + 0.5);

	} else {

	    ele_attr->surv_thr_parm = (int) (num*VCP_SNR_SCALE - 0.5);

	}
}

/************************************************************************
 *	Description: This function returns the velocity noise		*
 *		     threshold for a cut in the current VCP.		*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: noise threshold (dB)					*
 ************************************************************************/

float
hci_current_vcp_get_vel_noise_threshold (
int	cut
)
{
	int	status;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -999.9;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	return (float) (ele_attr->vel_thrsh_parm/VCP_SNR_SCALE);

}

/************************************************************************
 *	Description: This function sets the velocity noise		*
 *		     threshold for a cut in the current VCP.		*
 *									*
 *	Input:  cut - elevation cut number				*
 *		num - noise threshold (dB)				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_set_vel_noise_threshold (
int	cut,
float	num
)
{
	Ele_attr	*ele_attr;

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	if (num >= 0) {

	    ele_attr->vel_thrsh_parm = (int) (num*VCP_SNR_SCALE + 0.5);

	} else {

	    ele_attr->vel_thrsh_parm = (int) (num*VCP_SNR_SCALE - 0.5);

	}

}

/************************************************************************
 *	Description: This function returns the spectrum width noise	*
 *		     threshold for a cut in the current VCP.		*
 *									*
 *	Input:  cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: noise threshold (dB)					*
 ************************************************************************/

float
hci_current_vcp_get_spw_noise_threshold (
int	cut
)
{
	int	status;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -999.9;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	return (float) (ele_attr->spw_thrsh_parm/VCP_SNR_SCALE);

}

/************************************************************************
 *	Description: This function sets the velocity noise		*
 *		     threshold for a cut in the current VCP.		*
 *									*
 *	Input:  cut - elevation cut number				*
 *		num - noise threshold (dB)				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_set_spw_noise_threshold (
int	cut,
float	num
)
{
	Ele_attr	*ele_attr;

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	if (num >= 0) {

	    ele_attr->spw_thrsh_parm = (int) (num*VCP_SNR_SCALE + 0.5);

	} else {

	    ele_attr->spw_thrsh_parm = (int) (num*VCP_SNR_SCALE - 0.5);

	}

}

/************************************************************************
 *	Description: This function returns the sector azimuth angle	*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  sector - sector number (1, 2, or 3)			*
 *		cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: sector azimuth (deg)					*
 ************************************************************************/

float
hci_current_vcp_get_sector_azimuth (
int	sector,
int	cut
)
{
	int	status;
	float	angle;
	Vcp_struct	*vcp;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -1.0;

	    }
	}

	vcp      = (Vcp_struct *) &Vcp_info.current_vcp_table;
	ele_attr = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (sector) {

	    case 1 :

		angle = ele_attr->azi_ang_1 / VCP_AZIMUTH_SCALE;
		break;

	    case 2 :

		angle = ele_attr->azi_ang_2 / VCP_AZIMUTH_SCALE;
		break;

	    case 3 :

		angle = ele_attr->azi_ang_3 / VCP_AZIMUTH_SCALE;
		break;

	    default :

		angle = -1.0;
		break;

	}

	return angle;

}

/************************************************************************
 *	Description: This function returns the sector PRF number	*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  sector - sector number (1, 2, or 3)			*
 *		cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: sector PRF number					*
 ************************************************************************/

int
hci_current_vcp_get_sector_prf_num (
int	sector,
int	cut
)
{
	int	status;
	int	num;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -1;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	switch (sector) {

	    case 1 :

		num = ele_attr->dop_prf_num_1;
		break;

	    case 2 :

		num = ele_attr->dop_prf_num_2;
		break;

	    case 3 :

		num = ele_attr->dop_prf_num_3;
		break;

	    default :

		num = -1;
		break;

	}

	return num;

}

/************************************************************************
 *	Description: This function returns the sector pulse count	*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  sector - sector number (1, 2, or 3)			*
 *		cut - elevation cut number				*
 *	Output: NONE							*
 *	Return: sector pulse count					*
 ************************************************************************/

int
hci_current_vcp_get_sector_pulse_cnt (
int	sector,
int	cut
)
{
	int	status;
	int	num;
	Ele_attr	*ele_attr;

/*	Make sure the structure has been initialized.  If not, return	*
 *	with a negative number to indicate it was unsucessfull.		*/

	if (!Init_flag) {

	    status = hci_read_current_vcp_data ();

	    if (status <= 0) {

		return -1;

	    }
	}

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	switch (sector) {

	    case 1 :

		num = ele_attr->pulse_cnt_1;
		break;

	    case 2 :

		num = ele_attr->pulse_cnt_2;
		break;

	    case 3 :

		num = ele_attr->pulse_cnt_3;
		break;

	    default :

		num = -1;
		break;

	}

	return num;

}

/************************************************************************
 *	Description: This function sets the sector azimuth angle	*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  sector - sector number (1, 2, or 3)			*
 *		cut - elevation cut number				*
 *		azimuth - azimuth angle (deg)				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_set_sector_azimuth (
int	sector,
int	cut,
float	azimuth
)
{
	short	angle;
	Ele_attr	*ele_attr;

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	switch (sector) {

	    case 1 :

		angle = (short) (azimuth * 10);
		ele_attr->azi_ang_1 = angle;
		break;

	    case 2 :

		angle = (short) (azimuth * 10);
		ele_attr->azi_ang_2 = angle;
		break;

	    case 3 :

		angle = (short) (azimuth * 10);
		ele_attr->azi_ang_3 = angle;
		break;

	    default :

		break;

	}

	return;

}

/************************************************************************
 *	Description: This function sets the sector PRF number		*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  sector - sector number (1, 2, or 3)			*
 *		cut - elevation cut number				*
 *		num - PRF number					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_set_sector_prf_num (
int	sector,
int	cut,
int	num
)
{
	Ele_attr	*ele_attr;

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	switch (sector) {

	    case 1 :

		ele_attr->dop_prf_num_1 = num;
		break;

	    case 2 :

		ele_attr->dop_prf_num_2 = num;
		break;

	    case 3 :

		ele_attr->dop_prf_num_3 = num;
		break;

	    default :

		break;

	}

	return;

}

/************************************************************************
 *	Description: This function sets the sector pulse count		*
 *		     for a cut in the current VCP.			*
 *									*
 *	Input:  sector - sector number (1, 2, or 3)			*
 *		cut - elevation cut number				*
 *		num - pulse count					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_set_sector_pulse_cnt (
int	sector,
int	cut,
int	num
)
{
	Ele_attr	*ele_attr;

	ele_attr = (Ele_attr *) &Vcp_info.current_vcp_table.vcp_ele [cut];

	switch (sector) {

	    case 1 :

		ele_attr->pulse_cnt_1 = num;
		break;

	    case 2 :

		ele_attr->pulse_cnt_2 = num;
		break;

	    case 3 :

		ele_attr->pulse_cnt_3 = num;
		break;

	    default :

		break;

	}

	return;

}

/************************************************************************
 *	Description: This function returns the PRF value for a		*
 *		     specified PRF number.				*
 *									*
 *	Input:  prf_num - prf number					*
 *	Output: NONE							*
 *	Return: PRF value						*
 ************************************************************************/

float
hci_get_prf_value (
int	prf_num
)
{
	if ((prf_num < FIRST_PRF) || (prf_num > LAST_PRF)) {

	    return (-1);

	} else {

	    return (Prf_value [prf_num-1]);

	}
}

/************************************************************************
 *	Description: This function returns the unambiguous range for a	*
 *		     specified PRF number and delta PRI.		*
 *									*
 *	Input:  delta_pri - delta PRI					*
 *		prf_num - prf number					*
 *	Output: NONE							*
 *	Return: unambiguous range (km)					*
 ************************************************************************/

int
hci_get_unambiguous_range (
int	delta_pri,
int	prf_num
)
{
	if ((delta_pri < FIRST_DELTA_PRI) ||
	    (delta_pri > LAST_DELTA_PRI ) ||
	    (prf_num   < FIRST_PRF      ) ||
	    (prf_num   > LAST_PRF       )) {

	    return (-1);

	} else {

	    return Unambiguous_range [delta_pri-1][prf_num-1];

	}
}

/************************************************************************
 *      Description: This function returns the unambiguous range for a  *
 *                   specified PRF number and delta PRI for SPRT.       *
 *                                                                      *
 *      Input:  delta_pri - delta PRI                                   *
 *              prf_num - prf number                                    *
 *      Output: NONE                                                    *
 *      Return: unambiguous range (km)                                  *
 ************************************************************************/

int
hci_get_unambiguous_range_sprt (
int     delta_pri,
int     prf_num
)
{
        if ((delta_pri < FIRST_DELTA_PRI) ||
            (delta_pri > LAST_DELTA_PRI ) ||
            (prf_num   < FIRST_PRF      ) ||
            (prf_num   > LAST_PRF       )) {

            return (-1);

        } else {

            return Unambiguous_range_sprt [delta_pri-1][prf_num-1];

        }
}


/************************************************************************
 *	Description: This function returns the state of the local VCP	*
 *		     ITC update flag.  If it is 0, then the ITC has	*
 *		     updated since it was last read.  Otherwise, it	*
 *		     hasn't been updated.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: VCP update flag						*
 ************************************************************************/

int
hci_current_vcp_update_flag ()
{
	return (Init_flag);
}

/************************************************************************
 *	Description: This function is the LB notification callback when	*
 *		     the VCP ITC is updated.  If the local lock is set,	*
 *		     nothing is done.  If unlocked, then the update	*
 *		     flag is set (0) so the new data will be read the	*
 *		     next time one of the data access functions is	*
 *		     called.						*
 *									*
 *	Input:  lbfd - LB file descriptor				*
 *		msg_id - ID of message updated				*
 *		msg_info - message information				*
 *		arg - user data						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_current_vcp_register (
int	lbfd,
LB_id_t	msg_id,
int	msg_info,
void	*arg
)
{
/*	Only set the init flag if the lock is not set.  Otherwise, we	*
 *	ignore updates.							*/

	if( (!Lock_flag) && (msg_id == VOL_STAT_GSM_ID) ) {

	    Init_flag = 0;

	}
}

/************************************************************************
 *	Description: This function is used to set a local lock flag	*
 *		     which prevents the local VCP buffer to be updated	*
 *		     when the VCP ITC is updated.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_lock_current_vcp_data ()
{
	Lock_flag = 1;
}

/************************************************************************
 *	Description: This function is used to clear a local lock flag	*
 *		     allowing the local VCP buffer to be updated when	*
 *		     the VCP ITC is updated.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_unlock_current_vcp_data ()
{
	Lock_flag = 0;
}
