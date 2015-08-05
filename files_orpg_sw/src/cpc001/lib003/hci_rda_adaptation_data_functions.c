 /*
  * RCS info
  * $Author: ccalvert $
  * $Locker:  $
  * $Date: 2009/04/06 18:17:59 $
  * $Id: hci_rda_adaptation_data_functions.c,v 1.31 2009/04/06 18:17:59 ccalvert Exp $
  * $Revision: 1.31 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: hci_rda_adaptation_data_functions.c			*
 *									*
 *	Description:  This module contains a collection of routines	*
 *	used by the HCI to interface with RDA VCP adaptation data and	*
 *	RDA VCP data.							*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_rda_adaptation_data.h>
#include <hci_vcp_data.h>

/*	Global variables.						*/

static	Rdacnt	Rda_adapt;			/* Buffer containing RDA VCP adaptation 
						   data block. 		*/
static	int	Init_flag = 0;			/* Initialization flag 	*/
static  int	Rda_adapt_io_status = 0; 	/* Last read status 	*/

static  char 	*Rda_vcp = NULL;		/* Buffer containing RDA VCP message 
						   (include RDA/RPG message header). */
static	int	Rda_vcp_init_flag = 0;		/* Initialization flag 	*/
static  int	Rda_vcp_io_status = 0; 		/* Last read status 	*/
static	int	Rda_vcp_update_flag = 1;	/* LB notification flag */

/*	Static function prototypes. 					*/
void Lb_notify_handler( int fd, LB_id_t msgid, int msg_info, void *arg );


/************************************************************************
 *	Description: This function returns the status of the last RDA	*
 *		     adaptation data I/O operation.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: last I/O status						*
 ************************************************************************/

int hci_rda_adaptation_io_status()
{
	return(Rda_adapt_io_status);
}

/************************************************************************
 *	Description: This function returns the status of the last RDA	*
 *		     vcp data I/O operation.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: last I/O status						*
 ************************************************************************/

int hci_rda_vcp_io_status()
{
	return(Rda_vcp_io_status);
}

/************************************************************************
 *	Description: This function reads the RDA adaptation data block.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: read status (error if negative)				*
 ************************************************************************/

int
hci_read_rda_adaptation_data (
)
{
	int	status;
	static int write_perm_flag = 0;

	if( !write_perm_flag )
	{
	  write_perm_flag = 1;
	  ORPGDA_write_permission( ORPGDAT_ADAPTATION );
	}
	
	status = ORPGDA_read (
			ORPGDAT_ADAPTATION,
			(char *) &Rda_adapt,
			(int) sizeof (Rdacnt), RDACNT);
			
	if (status == LB_BUF_TOO_SMALL)
	   status = 0;
	    
	Rda_adapt_io_status = status;
	if (status < 0) {

	    HCI_LE_error("Initialization of ORPGDAT_ADAPTATION failed: %d",
			status);

	    return status;

	}

	Init_flag = 1;

	return status; 
}

/************************************************************************
 *      Description: This function reads the RDA VCP data. 		*
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: read status (error if negative)                         *
 ************************************************************************/

int
hci_read_rda_vcp_data (
)
{
        int     status;
	static	int size = sizeof(RDA_RPG_message_header_t) + sizeof(VCP_ICD_msg_t);

	if( !Rda_vcp_init_flag )
	{
	  /* Set up LB notification since this data is expected to change. */
	  ORPGDA_write_permission( ORPGDAT_RDA_VCP_DATA );
	  status = ORPGDA_UN_register( ORPGDAT_RDA_VCP_DATA, ORPGDAT_RDA_VCP_MSG_ID,
				     Lb_notify_handler );

	  if( status >= 0 )
	     Rda_vcp_init_flag = 1;
	  else
	  {
	   HCI_LE_error("ORPGDA_UN_register of ORPGDAT_RDA_VCP_DATA Failed: %d",
			status );
	  }
	}


	/* Allocate space for the largest possible VCP message. */

	if( Rda_vcp == NULL ){

           Rda_vcp = calloc( 1, size );
           if( Rda_vcp == NULL ){

              HCI_LE_error( "calloc Failed for %d Bytes", size );
	      return 0;

	   }

        }

	/* Read the VCP message. */

        status = ORPGDA_read ( ORPGDAT_RDA_VCP_DATA, (char *) Rda_vcp,
                               size, ORPGDAT_RDA_VCP_MSG_ID);

	/* This should never happen .... */

        if (status == LB_BUF_TOO_SMALL)
           status = 0;

	/* If read error, free space allocated to VCP data. */

        Rda_vcp_io_status = status;
        if (status <= 0) {

            HCI_LE_error("ORPGDA_read of ORPGDAT_RDA_VCP_DATA Failed: %d",
                        status);

	    free( Rda_vcp );
	    Rda_vcp = NULL;

            return status;

        }

	Rda_vcp_update_flag = 0;

        return status;
}

/************************************************************************
 *      Description: This function is the LB Notification handler for	*
 *	             RDA VCP data. 					*
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE
 ************************************************************************/

void Lb_notify_handler( int fd, LB_id_t msgid, int msg_info, void *arg ){

   Rda_vcp_update_flag = 1;

}

/************************************************************************
 *	Description: This function writes the RDA adaptation data	*
 *		     block.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: write status (error if negative)			*
 ************************************************************************/

int
hci_write_rda_adaptation_data (
)
{
	int	status;

	HCI_LE_status("Updating rda adaptation data");

	if (!Init_flag) 
	{
 	    
	    status = hci_read_rda_adaptation_data ();
	    Rda_adapt_io_status = status;
	    
	    if (status != 0) {

		return status;

	    }
	}

	status = ORPGDA_write (ORPGDAT_ADAPTATION,
			       (char *) &Rda_adapt,
			      sizeof (Rdacnt),
			      RDACNT);

	Rda_adapt_io_status = status;
	if (status <= 0) {

	    HCI_LE_error("ORPGDA_write of ORPGDAT_ADAPTATION failed: %d",
		status);

	}

	return status;

}


/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the Weather mode table in the RDA adaptation data	*
 *		     block.  This table is used to determine the wx	*
 *		     mode which corresponds to a defined VCP.  The VCP	*
 *		     number is contained in the table.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to wx mode table				*
 ************************************************************************/

short
*hci_rda_adapt_wxmode_table_ptr (
)
{
	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	return (short *) &Rda_adapt.rdcwxvcp [0][0];
}

/************************************************************************
 *	Description: This function returns the VCP number for the	*
 *		     specified Weather mode table entry.		*
 *									*
 *	Input:  mode - weather mode (0 - Clear Air; 1 - Precipitation)	*
 *		pos  - table index					*
 *	Output: NONE							*
 *	Return: VCP number						*
 ************************************************************************/

int
hci_rda_adapt_wxmode (
int	mode,
int	pos
)
{
	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	return  Rda_adapt.rdcwxvcp [mode][pos];
}


/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the Where Defined table in the RDA adaptation data	*
 *		     block.  This table is used to determine if a	*
 *		     VCP is defined at the RDA, RPG, or both.  The VCP	*
 *		     number is contained in the table.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to where defined table				*
 ************************************************************************/

short
*hci_rda_adapt_where_defined_table_ptr (
)
{
	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	return (short *) &Rda_adapt.rdc_where_defined [0][0];
}

/************************************************************************
 *	Description: This function returns the VCP number for the	*
 *		     specified Where Defined table entry.		*
 *									*
 *	Input:  comp - component (either VCP_RDA_DEFINED or 		*
 *		       VCP_RPG_DEFINED 					*
 *		vcp  - vcp number					*
 *	Output: NONE							*
 *	Return: 1 if TRUE, 0 if FALSE					*
 ************************************************************************/

int
hci_rda_adapt_where_defined (
int	comp,
int	vcp
)
{

	int i;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

        for (i=0; i<VCPMAX; i++) {

	   if ((Rda_adapt.rdc_where_defined [comp][i] & ORPGVCP_VCP_MASK) == vcp)
              return 1;

	}

	return 0;
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the data for a VCP in the RDA adaptation data	*
 *		     block.						*
 *									*
 *	Input:  pos - table position					*
 *	Output: NONE							*
 *	Return: pointer to VCP data					*
 ************************************************************************/

short
*hci_rda_adapt_vcp_table_ptr (
int	pos
)
{
	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	return (short *) &Rda_adapt.rdcvcpta [pos][0];
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the data for the RDA VCP data.			*
 *									*
 *	Output: NONE							*
 *	Return: pointer to VCP data					*
 ************************************************************************/

short
*hci_rda_vcp_ptr (
)
{
	/* Do we need to re-read the RDA VCP data? */

	if( (Rda_vcp_update_flag) 
		     || 
	    (!Rda_vcp_init_flag) 
		     ||
	    (Rda_vcp == NULL) ){

	    hci_read_rda_vcp_data ();

	}

	/* If read failed, return NULL.   Otherwise return pointer
	   to start of VCP data. */

        if( Rda_vcp == NULL )
           return NULL;

	return (short *) (Rda_vcp + sizeof(RDA_RPG_message_header_t));
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the RPG elevation indicies table in the RDA	*
 *		     adaptation data block.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to elevation indices table			*
 ************************************************************************/

short
*hci_rda_adapt_elev_indicies_ptr (
)
{
	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	return (short *) &Rda_adapt.rdccon [0][0];
}

/************************************************************************
 *	Description: This function returns the elevation angle 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input:  vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *	Output: NONE							*
 *	Return: elevation angle (degrees)				*
 ************************************************************************/

float
hci_rda_adapt_vcp_elevation_angle (
int	vcp_num,
int	cut
)
{
	int	indx;
	float	elevation;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	indx = hci_rda_adapt_vcp_table_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data then	*
 *	return a -1.							*/

	if ((indx < 0) || (cut > VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	elevation = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                         ele->ele_angle );

	return elevation;
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the allowable PRF table for the specified VCP	*
 *		     (referenced by position).				*
 *									*
 *	Input:  pos - position of VCP in VCP adaptation data table.	*
 *	Output: NONE							*
 *	Return: pointer to allowable PRF data for VCP			*
 ************************************************************************/

short
*hci_rda_adapt_allowable_prf_ptr (
int	pos
)
{

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	return	(short *) &Rda_adapt.alwblprf [pos][0];
}

/************************************************************************
 *	Description: This function returns the VCP number from the	*
 *		     specified allowable PRF table.			*
 *									*
 *	Input:  pos - position of VCP in VCP adaptation data table.	*
 *	Output: NONE							*
 *	Return: VCP number						*
 ************************************************************************/

int
hci_rda_adapt_allowable_prf_vcp_num (
int	pos
)
{
	Vcp_alwblprf_t	*alwblprf;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	alwblprf = (Vcp_alwblprf_t *) hci_rda_adapt_allowable_prf_ptr (pos);

	return alwblprf->vcp_num;
}

/************************************************************************
 *	Description: This function returns the number of allowable PRFs	*
 *		     for a specified allowable PRF table VCP.		*
 *									*
 *	Input:  pos - position of VCP in VCP adaptation data table.	*
 *	Output: NONE							*
 *	Return: number of allowable PRFs				*
 ************************************************************************/

int
hci_rda_adapt_allowable_prf_prfs (
int	pos
)
{
	Vcp_alwblprf_t	*alwblprf;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	alwblprf = (Vcp_alwblprf_t *) hci_rda_adapt_allowable_prf_ptr (pos);

	return alwblprf->num_alwbl_prf;

}

/************************************************************************
 *	Description: This function returns the PRF number of the	*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  pos  - position of VCP in VCP adaptation data table.	*
 *		indx - allowable PRF index (0 < X < num PRFs)		*
 *	Output: NONE							*
 *	Return: PRF number						*
 ************************************************************************/

int
hci_rda_adapt_allowable_prf_num (
int	pos,
int	indx
)
{
	Vcp_alwblprf_t	*alwblprf;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	alwblprf = (Vcp_alwblprf_t *) hci_rda_adapt_allowable_prf_ptr (pos);

	return alwblprf->prf_num [indx];
}

/************************************************************************
 *	Description: This function returns the pulse count of the	*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  pos      - position of VCP in VCP adaptation data table	*
 *		elev_num - elevation cut number				*
 *		prf      - PRF number					*
 *	Output: NONE							*
 *	Return: pulse count						*
 ************************************************************************/

int
hci_rda_adapt_allowable_prf_pulse_count (
int	pos,
int	elev_num,
int	prf
)
{
	Vcp_alwblprf_t	*alwblprf;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	alwblprf = (Vcp_alwblprf_t *) hci_rda_adapt_allowable_prf_ptr (pos);

	return (int) alwblprf->pulse_cnt [elev_num][prf-1];
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the PRF adaptation data in the RDA adaptataion	*
 *		     data block.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to PRF data					*
 ************************************************************************/

float
*hci_rda_adapt_prf_value_ptr (
)
{

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	return	(float *) &Rda_adapt.prfvalue [0];
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the VCP scan time table in the RDA adaptation data	*
 *		     block.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to VCP scan times				*
 ************************************************************************/

short
*hci_rda_adapt_vcp_times_ptr (
)
{

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	return (short *) &Rda_adapt.vcp_times [0];
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the unambiguous range table in the RDA adaptation	*
 *		     data block.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to unambiguous range data			*
 ************************************************************************/

int
*hci_rda_adapt_uambiguous_range_ptr (
)
{

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	return (int *) &Rda_adapt.unambigr [0][0];
}


/************************************************************************
 *	Description: This function tries to match the input VCP number	*
 *		     with an entry in the adaptation data.  If a match	*
 *		     is found, the position in the adaptation data is	*
 *		     returned.  If not, then a -1 is returned.		*
 *									*
 *	Input:  vcp_number - VCP number					*
 *	Output: NONE							*
 *	Return: position of VCP in VCP table.				*
 ************************************************************************/

int
hci_rda_adapt_vcp_table_index (
int	vcp_number
)
{
	int		i;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}

	for (i=0;i<VCPMAX;i++) {

	    if (vcp_number == hci_rda_adapt_vcp_table_vcp_num (i)) {

		return i;

	    }
	}

	return -1;

}

/************************************************************************
 *	Description: This function returns the VCP number for a		*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input:  pos - VCP position in table				*
 *	Output: NONE							*
 *	Return: VCP number						*
 ************************************************************************/

int
hci_rda_adapt_vcp_table_vcp_num (
int	pos
)
{
	Vcp_struct	*vcp;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	
	vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (pos);

	return (int) vcp->vcp_num;
}

/************************************************************************
 *	Description: This function returns the pulse width for a	*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input:  pos - VCP position in table				*
 *	Output: NONE							*
 *	Return: pulse width						*
 ************************************************************************/

int
hci_rda_adapt_vcp_table_pulse_width (
int	pos
)
{
	Vcp_struct	*vcp;

	if (!Init_flag) {

	    hci_read_rda_adaptation_data ();

	}
	
	vcp = (Vcp_struct *) hci_rda_adapt_vcp_table_ptr (pos);

	return (int) vcp->pulse_width;
}

/************************************************************************
 *	Description: This function returns the delta PRI value from the	*
 *		     RDA Control adaptation data block.  It first	*
 *		     attempts to get it from RDA adaptation data.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: delta PRI value						*
 ************************************************************************/

int
hci_rda_adapt_delta_pri (
)
{
        int ret, delta_pri = 0;

        ret = ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_DELTA_PRI, 
                                           (void *) &delta_pri );
        if( (ret == 0) && (delta_pri >= FIRST_DELTA_PRI)
                       && (delta_pri <= LAST_DELTA_PRI) )
           return( delta_pri );

        HCI_LE_error( "ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_DELTA_PRI ) Failed" );
        HCI_LE_error( "--->Returning value from RDA Control adaptation data: %d", Rda_adapt.delta_pri );
	return	Rda_adapt.delta_pri;
}
