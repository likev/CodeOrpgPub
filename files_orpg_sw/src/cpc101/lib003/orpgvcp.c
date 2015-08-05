/*
  * RCS info
  * $Author: steves $
  * $Locker:  $
  * $Date: 2014/02/11 17:08:57 $
  * $Id: orpgvcp.c,v 1.36 2014/02/11 17:08:57 steves Exp $
  * $Revision: 1.36 $
  * $State: Exp *
  */

/************************************************************************
 *									*
 *	Module: orpgvcp.c						*
 *									*
 *	Description:  This module contains a collection of functions	*
 *	dealing with VCP adaptation data.				*
 *									*
 *	Developer: David Priegnitz - CIMMS/NSSL				*
 *	Reference: RDA/RPG ICD message type 7				*
 *									*
 ************************************************************************/

/*	System include file definitions.				*/

#include <math.h>

/*	Local include file definitions.					*/

#include <infr.h>
#include <orpg.h>
#include <orpgda.h>
#include <orpgvcp.h>
#include <rpg_port.h>

/*	Global variables.						*/

static	Rdacnt	Vcp_adapt;	         /* Buffer containing rda adaptation data
				            block. */
static	int	Vcp_adapt_updated  = 0;	 /* 0 = RDACNT LB needs to be read.
					    1 = RDACNT LB does not need to
					       be read. */
static	int	Vcp_adapt_registered = 0; /* 0 = RDACNT LB notify not registered.
					     1 = RDACNT LB notify registered. */
static  int	Vcp_adapt_status  = 0;    /* Last read/write status for RDACNT */

static	RDA_rdacnt_t	Rda_vcp;	  /* Buffer containing rda VCP data. */
static	int	Rda_vcp_updated  = 0;	  /* 0 = RDA_RDACNT LB needs to be read.
					     1 = RDA_RDACNT LB does not need to
					         be read. */
static	int	Rda_vcp_registered = 0;   /* 0 = RDA_RDACNT LB notify not registered.
					     1 = RDA_RDACNT LB notify registered. */
static  int	Rda_vcp_status  = 0;      /* Last read/write status for RDA_RDACNT */


void ORPGVCP_adapt_lb_notify (int fd, LB_id_t msg_id, int msg_info, void *arg);
void ORPGVCP_rdavcp_lb_notify (int fd, LB_id_t msg_id, int msg_info, void *arg);

/************************************************************************
 *	Description: This function returns the status of the last VCP	*
 *		     adaptation data I/O operation.			*
 *									*
 *	Output: NONE							*
 *	Return: last I/O status						*
 ************************************************************************/
int ORPGVCP_io_status( ){

	return(Vcp_adapt_status);

/* End of ORPGVCP_io_status() */
}

/************************************************************************
 *	Description: This function sets the VCP	adaptation data LB	*
 *		     init flag whenever the VCP adaptation data LB	*
 *		     is updated.					*
 *									*
 *	Input:  NONE							*
 *	Output: fd       - file descriptor of LB that was updated	*
 *		msg_id   - message ID that was updated.			*
 *		msg_info - length of arg data				*
 *		*arg     - (unused)					*
 *	Return: NONE							*
 ************************************************************************/
void ORPGVCP_adapt_lb_notify( int fd, LB_id_t msg_id, int msg_info, void *arg ){

	Vcp_adapt_updated = 0;

/* End of ORPGVCP_adapt_lb_notify() */
}

/************************************************************************
 *	Description: This function sets the VCP	adaptation data LB	*
 *		     init flag whenever the VCP adaptation data LB	*
 *		     is updated.					*
 *									*
 *	Input:  NONE							*
 *	Output: fd       - file descriptor of LB that was updated	*
 *		msg_id   - message ID that was updated.			*
 *		msg_info - length of arg data				*
 *		*arg     - (unused)					*
 *	Return: NONE							*
 ************************************************************************/
void ORPGVCP_rdavcp_lb_notify( int fd, LB_id_t msg_id, int msg_info, void *arg ){

	Rda_vcp_updated = 0;

/* End of ORPGVCP_adapt_lb_notify() */
}
/************************************************************************
 *	Description: This function reads the RDA adaptation data block.	*
 *									*
 *	Output: NONE							*
 *	Return: read status (error if negative)				*
 ************************************************************************/

int ORPGVCP_read (){


/*	First check to see if VCP adaptation data LB		*
 *	notification registered.				*/

	if (!Vcp_adapt_registered) {

	    int	status;

	    status = ORPGDA_write_permission (ORPGDAT_ADAPTATION);

	    status = ORPGDA_UN_register( ORPGDAT_ADAPTATION, RDACNT, 
				         ORPGVCP_adapt_lb_notify );

	    if (status != LB_SUCCESS) {

		LE_send_msg (GL_ERROR,
		    "ORPGVCP: ORPGDA_UN_register (ORPGDAT_ADAPTATION, RDACNT) failed (ret (%d)", status);
		return (status);

	    }

	    Vcp_adapt_registered = 1;

	}

/*	Read the RDACNT block from RDA adaptation data LB.	*/

	Vcp_adapt_updated = 1;
	Vcp_adapt_status = ORPGDA_read( ORPGDAT_ADAPTATION,
		                        (char *) &Vcp_adapt,
		                        (int) sizeof (Rdacnt), RDACNT);
			
	if (Vcp_adapt_status < 0) {

	    Vcp_adapt_updated = 0;
	    LE_send_msg (GL_INFO,
		"ORPGVCP: ORPGDAT_ADAPTATION read failed: %d\n",
		Vcp_adapt_status);

	    return Vcp_adapt_status;

	}

	return Vcp_adapt_status; 

/* End of ORPGVCP_read() */
}

/************************************************************************
 *      Description: This function reads the RDA VCP data block.        *
 *                                                                      *
 *      Output: NONE                                                    *
 *      Return: read status (error if negative)                         *
 ************************************************************************/

int ORPGVCP_read_rdavcp (){


/*      First check to see if RDA VCP data LB notification 	*
	registered. 	 					*/

        if (!Rda_vcp_registered) {

            int status;

            status = ORPGDA_write_permission (ORPGDAT_ADAPTATION);

            status = ORPGDA_UN_register( ORPGDAT_ADAPTATION, RDA_RDACNT,
                                         ORPGVCP_rdavcp_lb_notify );

            if (status != LB_SUCCESS) {

                LE_send_msg (GL_ERROR,
                    "ORPGVCP: ORPGDA_UN_register (ORPGDAT_ADAPTATION, RDA_RDACNT) failed (ret (%d)", status);
                return (status);

            }

            Rda_vcp_registered = 1;

        }

/*      Read the RDACNT block from RDA adaptation data LB.      */

        Rda_vcp_updated = 1;
        Rda_vcp_status = ORPGDA_read( ORPGDAT_ADAPTATION,
                                      (char *) &Rda_vcp,
                                      (int) sizeof (RDA_rdacnt_t), RDA_RDACNT);

        if (Rda_vcp_status < 0) {

            Rda_vcp_updated = 0;
            LE_send_msg (GL_INFO, "ORPGVCP: ORPGDAT_ADAPTATION read failed: %d\n",
                Rda_vcp_status);

            return Rda_vcp_status;

        }

        return Rda_vcp_status;

/* End of ORPGVCP_read_rdavcp() */
}

/************************************************************************
 *	Description: This function writes the RDA adaptation data	*
 *		     block.						*
 *									*
 *	Output: NONE							*
 *	Return: write status (error if negative)			*
 ************************************************************************/
int ORPGVCP_write (){

	int	status;

	status = ORPGDA_write( ORPGDAT_ADAPTATION, (char *) &Vcp_adapt,
			       sizeof (Rdacnt), RDACNT);

	if (status <= 0)
	    LE_send_msg (GL_INFO,
		"ORPGVCP: ORPGDAT_ADAPTATION write failed: %d\n",
		status);

	return status;

/* End of ORPGVCP_write() */
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
short* ORPGVCP_wxmode_tbl_ptr(){

	if( !Vcp_adapt_updated )
	    ORPGVCP_read ();

	return (short *) &Vcp_adapt.rdcwxvcp [0][0];

/* End of ORPGVCP_wxmode_tbl_ptr() */
}

/************************************************************************
 *	Description: This function returns the VCP number for the	*
 *		     specified Weather mode table entry.		*
 *									*
 *	Input:  wx_mode - weather mode (CLEAR_AIR_MODE;			*
 *					PRECIPITATION_MODE)		*
 *		pos  - table index					*
 *	Output: NONE							*
 *	Return: VCP number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_wxmode_vcp( int	wx_mode, int pos){

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((wx_mode < CLEAR_AIR_MODE) || (wx_mode > PRECIPITATION_MODE)) {

	    return (-1);

	}

	if ((pos < 0) || (pos >= 20)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated)
	    ORPGVCP_read();

	return  Vcp_adapt.rdcwxvcp [wx_mode-1][pos];

/* End of ORPGVCP_get_wxmode_vcp() */
}

/************************************************************************
 *	Description: This function sets the VCP number for the		*
 *		     specified Weather mode table entry.		*
 *									*
 *	Input:  wx_mode - weather mode (CLEAR_AIR_MODE;			*
 *					PRECIPITATION_MODE)		*
 *		pos  - table index					*
 *		vcp  - VCP number					*
 *	Output: NONE							*
 *	Return: VCP number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_wxmode_vcp ( int wx_mode, int pos, int vcp ){

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((wx_mode < CLEAR_AIR_MODE) || (wx_mode > PRECIPITATION_MODE)) {

	    return (-1);

	}

/*	Validate the input VCP number.  According to the RDA-RPG ICD,	*
 *	it should be a positive value.					*/

	if ((vcp < 0) || (vcp > 32768)) {

	    return (-1);

	}

	Vcp_adapt.rdcwxvcp [wx_mode-1][pos] = (short) vcp;

	return (vcp);

/* End of ORPGVCP_set_wxmode_vcp() */
}

/************************************************************************
 *      Description: This function returns the value for where the      *
 *                   VCP is defined.                                    *
 *                                                                      *
 *      Input:  where_defined                                           *
 *              pos  - table index                                      * 
 *      Output: NONE                                                    *
 *      Return: where defined value on success, -1 on error             *
 ************************************************************************/
int ORPGVCP_get_where_defined_vcp ( int where_defined, int pos ){

/*      Validate the input arguments.  If out of range return -1.       */

        if ((((where_defined & ORPGVCP_RDA_DEFINED_VCP) != ORPGVCP_RDA_DEFINED_VCP))
                                       && 
	    (((where_defined & ORPGVCP_RPG_DEFINED_VCP) != ORPGVCP_RPG_DEFINED_VCP))) {

            return (-1);

        }

        if ((pos < 0) || (pos >= 20)) {

            return (-1);

        }

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated)
            ORPGVCP_read ();

        return ( Vcp_adapt.rdc_where_defined[where_defined-1][pos] & 
                 ORPGVCP_VCP_MASK );

/* End of ORPGVCP_get_where_defined_vcp() */
}

/************************************************************************
 *      Description: This function returns the value for where the      *
 *                   VCP is defined.                                    *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *      Output: NONE                                                    *
 *      Return: where defined value on success, -1 on error             *
 ************************************************************************/
int ORPGVCP_get_where_defined( int vcp_num ){

	int pos;
        int retval = 0;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated)
            ORPGVCP_read ();

	/* Look through the RPG and RDA defined VCPs for a match. 	*/
        for( pos = 0; pos < 20; pos++ ){

	    if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP, 
                                               pos ) == vcp_num )
               retval |= ORPGVCP_RPG_DEFINED_VCP;

	    if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP, 
                                               pos ) == vcp_num )
               retval |= ORPGVCP_RDA_DEFINED_VCP;

        }
	
        if( retval != 0 )
           return (retval );

	return (-1);

/* End of ORPGVCP_get_where_defined() */
}

/************************************************************************
 *      Description: This function returns the value for whether or     *
 *                   not the VCP is VCP data message defined.           *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *      Output: NONE                                                    *
 *      Return: 0 is not, 1 is yes, -1 on error                         *
 ************************************************************************/
int ORPGVCP_is_vcp_data_defined( int vcp_num ){

        int pos;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated)
            ORPGVCP_read ();

        /* Look through the RPG and RDA defined VCPs for a match.       */
        for( pos = 0; pos < 20; pos++ ){

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP,
                                               pos ) == vcp_num )
               return ( Vcp_adapt.rdc_where_defined[ORPGVCP_RPG_DEFINED_VCP-1][pos] &
                        ORPGVCP_DATA_DEFINED_VCP );

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP,
                                               pos ) == vcp_num )
               return ( Vcp_adapt.rdc_where_defined[ORPGVCP_RDA_DEFINED_VCP-1][pos] &
                        ORPGVCP_DATA_DEFINED_VCP );

        }

        return (-1);

/* End of ORPGVCP_is_vcp_data_defined() */
}

/************************************************************************
 *      Description: This function sets whether or not the VCP is       *
 *                   data defined.                                      * 
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *      Output: NONE                                                    *
 *      Return: 0 on success, -1 on error                               *
 ************************************************************************/
int ORPGVCP_set_vcp_is_data_defined( int vcp_num ){

        int pos;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated)
            ORPGVCP_read ();

        /* Look through the RPG and RDA defined VCPs for a match.       */
        for( pos = 0; pos < 20; pos++ ){

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP,
                                               pos ) == vcp_num )
            {
               Vcp_adapt.rdc_where_defined[ORPGVCP_RPG_DEFINED_VCP-1][pos] |=
                                                   ORPGVCP_DATA_DEFINED_VCP;
               return (0);

            }

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP,
                                               pos ) == vcp_num )
            {
               Vcp_adapt.rdc_where_defined[ORPGVCP_RDA_DEFINED_VCP-1][pos] |=
                                                   ORPGVCP_DATA_DEFINED_VCP;

               return (0);

            }

        }

        return (-1);

/* End of ORPGVCP_set_vcp_data_defined() */
}

/************************************************************************
 *      Description: This function returns the value for whether or     *
 *                   not the VCP is site specific.                      *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *		where_define - either ORPGVCP_RDA_DEFINED_VCP or	*
 *                             ORPGVCP_RPG_DEFINED_VCP			*
 *      Output: NONE                                                    *
 *      Return: 0 is not, 1 is yes, -1 on error                         *
 ************************************************************************/
int ORPGVCP_is_vcp_site_specific( int vcp_num, int where_defined ){

        int pos;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated)
            ORPGVCP_read ();

        /* Look through the RPG and RDA defined VCPs for a match.       */
        for( pos = 0; pos < 20; pos++ ){

            if( ORPGVCP_get_where_defined_vcp( where_defined, pos )
                                               == vcp_num ){

               if( where_defined == ORPGVCP_RPG_DEFINED_VCP )
                  return ( Vcp_adapt.rdc_where_defined[where_defined-1][pos] &
                        ORPGVCP_SITE_SPECIFIC_RPG_VCP );

               if( where_defined == ORPGVCP_RDA_DEFINED_VCP )
                  return ( Vcp_adapt.rdc_where_defined[where_defined-1][pos] &
                        ORPGVCP_SITE_SPECIFIC_RDA_VCP );
            }

        }

        return (-1);

/* End of ORPGVCP_is_vcp_site_specific() */
}

/************************************************************************
 *      Description: This function sets whether or not the VCP is       *
 *                   site specific                                      * 
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *		where_define - either ORPGVCP_RDA_DEFINED_VCP or	*
 *                             ORPGVCP_RPG_DEFINED_VCP			*
 *      Output: NONE                                                    *
 *      Return: 0 on success, -1 on error                               *
 ************************************************************************/
int ORPGVCP_set_vcp_site_specific( int vcp_num, int where_defined ){

        int pos;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated)
            ORPGVCP_read ();

        /* Look through the RPG and RDA defined VCPs for a match.       */
        for( pos = 0; pos < 20; pos++ ){

            if( ORPGVCP_get_where_defined_vcp( where_defined, pos ) == vcp_num )
            {

               if( where_defined == ORPGVCP_RPG_DEFINED_VCP )
                  Vcp_adapt.rdc_where_defined[where_defined-1][pos] |=
                                                   ORPGVCP_SITE_SPECIFIC_RPG_VCP;

               else if( where_defined == ORPGVCP_RDA_DEFINED_VCP )
                  Vcp_adapt.rdc_where_defined[where_defined-1][pos] |=
                                                   ORPGVCP_SITE_SPECIFIC_RDA_VCP;
               return (0);

            }

        }

        return (-1);

/* End of ORPGVCP_set_vcp_site_specific() */
}

/************************************************************************
 *      Description: This function returns the value for whether or     *
 *                   not the VCP is experimental.                       *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *      Output: NONE                                                    *
 *      Return: 0 is not, 1 is yes, -1 on error             		*
 ************************************************************************/
int ORPGVCP_is_vcp_experimental( int vcp_num ){

        int pos;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated)
            ORPGVCP_read ();

        /* Look through the RPG and RDA defined VCPs for a match.       */
        for( pos = 0; pos < 20; pos++ ){

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP,
                                               pos ) == vcp_num )
               return ( Vcp_adapt.rdc_where_defined[ORPGVCP_RPG_DEFINED_VCP-1][pos] &
                        ORPGVCP_EXPERIMENTAL_VCP );

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP,
                                               pos ) == vcp_num )
               return ( Vcp_adapt.rdc_where_defined[ORPGVCP_RDA_DEFINED_VCP-1][pos] &
                        ORPGVCP_EXPERIMENTAL_VCP );

        }

        return (-1);

/* End of ORPGVCP_is_vcp_experimental() */
}

/************************************************************************
 *      Description: This function sets whether or not the VCP is 	*
 *		     experimental.		                      	* 
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *      Output: NONE                                                    *
 *      Return: 0 on success, -1 on error 	                        *
 ************************************************************************/
int ORPGVCP_set_vcp_is_experimental( int vcp_num ){

        int pos;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated) 
            ORPGVCP_read ();

        /* Look through the RPG and RDA defined VCPs for a match.       */
        for( pos = 0; pos < 20; pos++ ){

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP,
                                               pos ) == vcp_num )
            {
               Vcp_adapt.rdc_where_defined[ORPGVCP_RPG_DEFINED_VCP-1][pos] |=
                                                   ORPGVCP_EXPERIMENTAL_VCP;
               return (0);

            }

            if( ORPGVCP_get_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP,
                                               pos ) == vcp_num )
            {
               Vcp_adapt.rdc_where_defined[ORPGVCP_RDA_DEFINED_VCP-1][pos] |=
                                                   ORPGVCP_EXPERIMENTAL_VCP;

               return (0);

            }

        }

        return (-1);

/* End of ORPGVCP_set_vcp_experimental() */
}

/************************************************************************
 *      Description: This function sets the VCP number for the          *
 *                   specified Weather mode table entry.                *
 *                                                                      *
 *      Input:  where_defined - (ORPGVCP_RDA_DEFINED_VCP;               *
 *                               ORPGVCP_RPG_DEFINED_VCP)               *
 *              pos  - position in the where-defined table		*
 *              vcp  - VCP number                                       *
 *      Output: NONE                                                    *
 *      Return: VCP number on success, -1 on error                      *
 ************************************************************************/
int ORPGVCP_set_where_defined_vcp( int where_defined, int pos, int vcp ){

/*      Validate the input VCP number.  According to the RDA-RPG ICD,   *
 *      it should be a positive value.                                  */

        if ((vcp < 0) || (vcp > 32768)) {

            return (-1);

        }
                
        Vcp_adapt.rdc_where_defined [where_defined-1][pos] = (short) vcp;
        LE_send_msg (GL_INFO,
                "Vcp_adapt.rdc_where_defined [%d][%d] = (short) %d;",
                where_defined-1, pos, Vcp_adapt.rdc_where_defined [where_defined-1][pos]);

                
        return (vcp);

/* End of ORPGVCP_set_where_defined_vcp() */
}           

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the data for a VCP in the RDA adaptation data	*
 *		     block or current VCP data buffer.			*
 *									*
 *		pos - table position					*
 *	Output: NONE							*
 *	Return: pointer to VCP data on success, NULL on error		*
 ************************************************************************/
short* ORPGVCP_ptr ( int pos ){

/*	Validate the input arguments.  If out of range return	*
 *	-1.							*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return ((short *) NULL);

	}

/*	If the data has changed in the file then we need to re-	*
 *	read it.						*/

	if (!Vcp_adapt_updated)
	    ORPGVCP_read ();

	return (short *) &Vcp_adapt.rdcvcpta [pos][0];

/* End of ORPGVCP_ptr() */
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the RPG elevation indicies table in the RDA	*
 *		     adaptation data block.				*
 *									*
 *	Input:  pos - position in the rdccon array			*
 *	Output: NONE							*
 *	Return: pointer to elevation indices table			*
 ************************************************************************/
short* ORPGVCP_elev_indicies_ptr(int pos){


/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

	return (short *) &Vcp_adapt.rdccon [pos][0];

/* End of ORPGVCP_elev_indicies_ptr() */
}

/************************************************************************
 *	Description: This function gets the RPG elevation number	*
 *		     of the specified elevation cut.			*
 *									*
 *	Input:  vcp_num - VCP number					*
 *		cut	- VCP cut number				*
 *		vol_num	- Volume Scan Number (optional)			*
 *	Output: NONE							*
 *	Return: RPG elevation number on success, -1 on error		*
 ************************************************************************/
int ORPGVCP_get_rpg_elevation_num ( int	vcp_num, int cut, ... ){

	int	elev_num;
	int	indx;
	VCP_ICD_msg_t *rda_vcp;
	int	rda_vcp_num;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	unsigned int vol_num = 0;

	va_list ap;

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

        if (rdavcp_flag && !Rda_vcp_updated )
	    ORPGVCP_read_rdavcp ();
           
	else if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*	If RDA VCP, then ..... 						*/

	if( rdavcp_flag ){

/*	Set up for volume scan number as optional argument.		*/

	    if( volnum_flag ){

		va_start( ap, cut );
		vol_num = va_arg( ap, unsigned int );
		vol_num %= 2;
		va_end( ap );

	    } else {

		vol_num = (unsigned int) Rda_vcp.last_entry;
	    }
		
/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/*	Extract elevation number. 					*/

	    if( cut < 0 ){

		elev_num = -1;

	    } else {
		
		elev_num = Rda_vcp.data[vol_num].rdccon[cut];
	    }

	    return (elev_num);

	}

/* 	Get information from adaptation data.  				*/

	indx = ORPGVCP_index (vcp_num);

	if (indx < 0) {

	    elev_num = -1;

	} else {

	    if (cut < 0) {

		elev_num = -1;

	    } else {

		elev_num = Vcp_adapt.rdccon[indx][cut];

	    }
	}

	return (elev_num);

/* End of ORPGVCP_get_rpg_elevation_num() */
}

/************************************************************************
 *	Description: This function sets the RPG elevation number	*
 *		     of the specified elevation cut.			*
 *									*
 *	Input:  vcp_num - VCP number					*
 *		cut	- VCP cut number				*
 *		num	- RPG elevation index				*
 *	Output: NONE							*
 *	Return: RPG elevation number on success, -1 on error		*
 ************************************************************************/
int ORPGVCP_set_rpg_elevation_num( int vcp_num, int cut, int num ){

	int	elev_num;
	int	indx;

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

	indx = ORPGVCP_index (vcp_num);

	if (indx < 0) {

	    elev_num = -1;

	} else {

	    if (cut < 0) {

		elev_num = -1;

	    } else {

		Vcp_adapt.rdccon [indx][cut] = num;
		elev_num = num;

	    }
	}

	return (elev_num);

/* End of ORPGVCP_set_rpg_elevation_num() */
}

/************************************************************************
 *	Description: This function returns the elevation angle 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input:	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		vol_num - volume number (optional)			*
 *	Output: NONE							*
 *	Return: elevation angle (degrees) on success, -99.9 on error	*
 ************************************************************************/
float ORPGVCP_get_elevation_angle( int vcp_num, int cut, ... ){

	int	indx;
	float	elevation;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
        VCP_ICD_msg_t   *rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*	If the data has changed in the file then we need to 		*
	re-read the datastore.						*/

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

/*	Make sure the cut number is valid. 				*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-99.9);

	}

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, cut );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-99.9);

/*	Extract the elevation angle, in deg.				*/

	    elevation = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                             rda_vcp->vcp_elev_data.data[cut].angle );
	    return (elevation);

	} 

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -99.9.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-99.9);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	elevation = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                         ele->ele_angle );
	return (elevation);

/* End of ORPGVCP_get_elevation_angle() */
}

/************************************************************************
 *	Description: This function sets the elevation angle 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input:	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		angle   - elevation angle				*
 *	Output: NONE							*
 *	Return: elevation angle (degrees) on success, -99.9 on error	*
 ************************************************************************/
float ORPGVCP_set_elevation_angle ( int vcp_num, int cut, float angle ){

	int	indx;
	float	elevation = angle;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

/*	Verify that the input elevation angle is within the allowed	*
 *	limits.								*/

	if (angle < -1.0) {

	    return (-99.9);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-99.9);

	}


	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -99.9.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-99.9);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];
	ele->ele_angle = (short) ORPGVCP_deg_to_BAMS( ORPGVCP_ELEVATION_ANGLE,
                                                      angle );

	return (elevation);

/* End of ORPGVCP_set_elevation_angle() */
}

/************************************************************************
 *	Description: This function returns the waveform type 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		vol_num - Volume number (optional)			*
 *	Output: NONE							*
 *	Return: VCP_WAVEFORM_UNKNOWN					*
 *		VCP_WAVEFORM_CS						*
 *		VCP_WAVEFORM_CD						*
 *		VCP_WAVEFORM_CDBATCH					*
 *		VCP_WAVEFORM_BATCH					*
 *		VCP_WAVEFORM_STP, or					*
 *		-1 on error						*
 ************************************************************************/
int ORPGVCP_get_waveform( int vcp_num, int cut, ... ){

	int	indx;
	int	waveform;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int 	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

/*	Validate the cut number. 				*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, cut );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/* 	Extract waveform.						*/

	    waveform = rda_vcp->vcp_elev_data.data[cut].waveform;
            return (waveform);

        }

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);


	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	waveform = ele->wave_type;

	return (waveform);

/* End of ORPGVCP_get_waveform() */
}

/************************************************************************
 *	Description: This function sets the waveform type 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		wave    - VCP_WAVEFORM_UNKNOWN				*
 *			  VCP_WAVEFORM_CS				*
 *			  VCP_WAVEFORM_CD				*
 *			  VCP_WAVEFORM_CDBATCH				*
 *			  VCP_WAVEFORM_BATCH				*
 *			  VCP_WAVEFORM_STP, or				*
 *			  -1 on error					*
 *	Output: NONE							*
 *	Return: coded waveform						*
 ************************************************************************/
int ORPGVCP_set_waveform( int vcp_num, int cut, int wave ){

	int	indx;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
        short 		wave_form;

	wave_form = (short) (wave & 0xff);

/*	Verify that the input waveform is an allowable value.		*/

	if ((wave_form != VCP_WAVEFORM_UNKNOWN) &&
	    (wave_form != VCP_WAVEFORM_CS)      &&
	    (wave_form != VCP_WAVEFORM_CD)      &&
	    (wave_form != VCP_WAVEFORM_CDBATCH) &&
	    (wave_form != VCP_WAVEFORM_BATCH)   &&
	    (wave_form != VCP_WAVEFORM_STP)) {

	    return (-1);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}


	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	ele->wave_type = (unsigned char) wave_form;

	return (wave_form);

/* End of ORPGVCP_set_waveform() */
}

/************************************************************************
 *	Description: This function returns the phase type 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: VCP_PHASE_CONSTANT					*
 *		VCP_PHASE_SZ2, or					*
 *		-1 on error						*
 ************************************************************************/
int ORPGVCP_get_phase_type( int vcp_num, int cut, ... ){

	int	indx;
	int	phase;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated)
	    ORPGVCP_read ();

/*	Validate the cut number is in range. 				*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, cut );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/*	Extract phase.							*/

	    phase = rda_vcp->vcp_elev_data.data[cut].phase;
            return (phase);

        }

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);


	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	phase = ele->phase;

	return (phase);

/* End of ORPGVCP_get_phase_type() */
}

/************************************************************************
 *	Description: This function returns true (1) if the input vcp	*
 *		     contains SZ-2 cuts or 0 otherwise.               	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *			             					*
 *	Output: NONE							*
 *	Return: 1 if SZ-2 VCP, 0 if not, or -1 on error.		*
 ************************************************************************/
int ORPGVCP_is_SZ2_vcp( int vcp_num ){

   int cut, phase, num_elevs;

   /* Get the number of cuts in this VCP. */
   num_elevs = ORPGVCP_get_num_elevations( vcp_num );
   if( num_elevs < 0 )
      return -1;

   /* Check each cut.   Return 1 if any SZ2 cut found. */
   for( cut = 0; cut < num_elevs; cut++ ){

      if( (phase = ORPGVCP_get_phase_type( vcp_num, cut )) < 0 )
         return -1;

      if( phase == ORPGVCP_SZ2_PHASE )
         return 1;

   }

   return 0;

/* End of ORPGVCP_is_SZ2_vcp(). */
}

/************************************************************************
 *	Description: This function sets the phase type	 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		phase   - VCP_PHASE_CONSTANT  				*
 *			  VCP_PHASE_SZ2  				*
 *			  -1 on error					*
 *	Output: NONE							*
 *	Return: coded phase						*
 ************************************************************************/
int ORPGVCP_set_phase_type ( int vcp_num, int cut, int phase ){

	int	indx;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

/*	Verify that the input phase is an allowable value.		*/

	if ((phase != VCP_PHASE_SZ2) &&
	    (phase != VCP_PHASE_CONSTANT)) {

	    return (-1);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}


	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	ele->phase = (unsigned char) (phase & 0xff);

	return (phase);

/* End of ORPGVCP_set_phase_type() */
}

/************************************************************************
 *      Description: This function returns the super res value          *
 *                   associated with a selected VCP and elevation cut.  *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *              cut     - elevation cut number                          *
 *		vol_num - volume scan number (optional)			*
 *      Output: NONE                                                    *
 *      Return: VCP_HALFDEG_RAD,VCP_QRTKM_SURV or VCP_300KM_DOP         *
 *              -1 on error                                             *
 ************************************************************************/
int ORPGVCP_get_super_res( int vcp_num, int cut, ...){

        int     indx;
        int     super_res;
        Vcp_struct      *vcp;
        Ele_attr        *ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

        else if (!Vcp_adapt_updated)
            ORPGVCP_read ();

/*	Validate the cut number is within range. 			*/
        if ((cut < 0) || (cut >= ECUTMAX)) {

            return (-1);

        }

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, cut );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/*	Extract super resolution.					*/

	    super_res = rda_vcp->vcp_elev_data.data[cut].super_res;
            super_res &= (0xff & VCP_SUPER_RES_MASK);   
        
            return (super_res);

        }

        indx = ORPGVCP_index (vcp_num);

/*      if the selected VCP is not found in the adaptation data 	*
 *      then return a -1.                                       	*/

        if ((indx < 0) || (indx >= VCPMAX)) {

            return (-1);

        }

        vcp = (Vcp_struct *) ORPGVCP_ptr (indx);


        ele = (Ele_attr *) &vcp->vcp_ele [cut];

        super_res = (int) (ele->super_res & (0xff & VCP_SUPER_RES_MASK));

        return (super_res);

/* End of ORPGVCP_get_super_res() */
}


/************************************************************************
 *      Description: This function returns the Dual Pol value           *
 *                   associated with a selected VCP and elevation cut.  *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *              cut     - elevation cut number                          *
 *		vol_num - volume scan number (optional)			*
 *      Output: NONE                                                    *
 *      Return: 1 is Dual Pol enabled, 0 if not enabled, or             *
 *              -1 on error                                             *
 ************************************************************************/
int ORPGVCP_get_dual_pol( int vcp_num, int cut, ... ){

        int     indx;
        int     dual_pol;
        Vcp_struct      *vcp;
        Ele_attr        *ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

        else if (!Vcp_adapt_updated)
            ORPGVCP_read ();

/*	Verify the cut number is within range.				*/

        if ((cut < 0) || (cut >= ECUTMAX)) {

            return (-1);

        }

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, cut );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/* 	Extract dual pol.						*/
	    dual_pol = rda_vcp->vcp_elev_data.data[cut].super_res & (0xff & VCP_DUAL_POL_ENABLED);
            return (dual_pol);

        }

        indx = ORPGVCP_index (vcp_num);

/*      if the selected VCP is not found in the adaptation data 	*
 *      then return a -1.                                       	*/

        if ((indx < 0) || (indx >= VCPMAX)) {

            return (-1);

        }

        vcp = (Vcp_struct *) ORPGVCP_ptr (indx);


        ele = (Ele_attr *) &vcp->vcp_ele [cut];

        dual_pol = (ele->super_res & (0xff & VCP_DUAL_POL_ENABLED));

        return (dual_pol);

/* End of ORPGVCP_get_dual_pol() */
}


/************************************************************************
 *      Description: This function sets the super res word              *
 *                   associated with a selected VCP and elevation cut.  *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *              cut     - elevation cut number                          *
 *              super_res - VCP_HALFDEG_RAD, VCP_QRTKM_SURV,            *
 *                          VCP_300KM_SZ2 or 0                          *
 *                                                                      *
 *      Output: NONE                                                    *
 *      Return: super_res                                               *
 ************************************************************************/
int ORPGVCP_set_super_res ( int vcp_num, int cut, int super_res ){

        int     indx;
        Vcp_struct      *vcp;
        Ele_attr        *ele;

/*      Verify that the input is an allowable value.              */

        if ((super_res < 0) 
                 || 
            (super_res > (VCP_HALFDEG_RAD | VCP_QRTKM_SURV | VCP_300KM_DOP))) {

            return (-1);

        }

        if ((cut < 0) || (cut >= ECUTMAX)) {

            return (-1);

        }


        indx = ORPGVCP_index (vcp_num);

/*      if the selected VCP is not found in the adaptation data *
 *      then return a -1.                                       */

        if ((indx < 0) || (indx >= VCPMAX)) {

            return (-1);

        }

        vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

        ele = (Ele_attr *) &vcp->vcp_ele [cut];

        ele->super_res |= super_res;

        return (super_res);

/* End of ORPGVCP_set_super_res() */
}

/************************************************************************
 *      Description: This function sets the dual pol word               *
 *                   associated with a selected VCP and elevation cut.  *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *              cut     - elevation cut number                          *
 *              dual_pol - VCP_DUAL_POL_ENABLED or 0                    *
 *                                                                      *
 *      Output: NONE                                                    *
 *      Return: dual_pol                                                *
 ************************************************************************/
int ORPGVCP_set_dual_pol ( int vcp_num, int cut, int dual_pol ){

        int     indx;
        Vcp_struct      *vcp;
        Ele_attr        *ele;

/*      Verify that the input is an allowable value.              */

        if ((dual_pol < 0) 
                 ||
            ((dual_pol != VCP_DUAL_POL_ENABLED) && (dual_pol != 0)) ) {

            return (-1);

        }

        if ((cut < 0) || (cut >= ECUTMAX)) {

            return (-1);

        }


        indx = ORPGVCP_index (vcp_num);

/*      if the selected VCP is not found in the adaptation data *
 *      then return a -1.                                       */

        if ((indx < 0) || (indx >= VCPMAX)) {

            return (-1);

        }

        vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

        ele = (Ele_attr *) &vcp->vcp_ele [cut];

        ele->super_res |= dual_pol;

        return (dual_pol);

/* End of ORPGVCP_set_dual_pol() */
}

/************************************************************************
 *	Description: This function returns the configuration type	*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		vol_num - volume scan number				*
 *	Output: NONE							*
 *	Return: ORPGVCP_LINEAR_CHANNEL,					*
 *		ORPGVCP_LOG_CHANNEL, or					*
 *		-1 on error						*
 ************************************************************************/
int ORPGVCP_get_configuration( int vcp_num, int cut, ... ){

	int	indx;
	int	configuration;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

/* 	Verify cut is within range. 					*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, cut );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/*	Extract configuration.						*/

	    configuration = rda_vcp->vcp_elev_data.data[cut].waveform & 0x8000;
            return (configuration);

        }

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data		*
 *	then return a -1.						*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);


	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	configuration = (ele->wave_type & 0x8000) >> 8;

	return (configuration);

/* End of ORPGVCP_get_configuration() */
}

/************************************************************************
 *	Description: This function sets the waveform type 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		config  - ORPGVCP_LINEAR_CHANNEL or			*
 *	 		  ORPGVCP_LOG_CHANNEL				*
 *	Output: NONE							*
 *	Return: config on success, -1 on error				*
 ************************************************************************/
int ORPGVCP_set_configuration( int vcp_num, int cut, int config ){

	int	indx;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

	if ((config != ORPGVCP_LINEAR_CHANNEL) &&
	    (config != ORPGVCP_LOG_CHANNEL)) {

	    return (-1);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

/*	The configuration is stored in the upper byte so we	*
 *	need to shift it and add it to the waveform type.	*/

	ele->wave_type = (ele->wave_type & 0x7fff) | ((short) config <<8);

	return (config);

/* End of ORPGVCP_set_configuration() */
}

/************************************************************************
 *	Description: This function returns the PRF number		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num -  VCP number					*
 *		cut     -  elevation cut number				*
 *		prf_type - ORPGVCP_SURVEILLANCE				*
 *			   ORPGVCP_DOPPLER1				*
 *			   ORPGVCP_DOPPLER2				*
 *			   ORPGVCP_DOPPLER3				*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: PRF number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_prf_num( int vcp_num, int cut, int type, ... ){

	int	indx;
	int	prf_num;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_elevation_cut_data_t *rda_ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*	Validate the prf_type. 					*/

	if ((type != ORPGVCP_SURVEILLANCE) &&
	    (type != ORPGVCP_DOPPLER1) &&
	    (type != ORPGVCP_DOPPLER2) &&
	    (type != ORPGVCP_DOPPLER3)) {

	    return (-1);

	}

/*	Validate the cut number is within range. 		*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

/*      If the data has changed in the file then we need 	*
	to re-read it.                                          */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated)
	    ORPGVCP_read ();

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, type );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/* 	Extract PRF number.						*/

	    rda_ele = (VCP_elevation_cut_data_t *) &rda_vcp->vcp_elev_data.data[cut];

	    switch (type) {

	   	 case ORPGVCP_SURVEILLANCE :

		    prf_num = rda_ele->surv_prf_num;
		    break;

	    	case ORPGVCP_DOPPLER1 :

		    prf_num = rda_ele->dopp_prf_num1;
		    break;

	    	case ORPGVCP_DOPPLER2 :

		    prf_num = rda_ele->dopp_prf_num2;
		    break;

	    	case ORPGVCP_DOPPLER3 :

		    prf_num = rda_ele->dopp_prf_num3;
		    break;

	    	default :

		    prf_num = -1;
		    break;

	    }

	    return (prf_num);

        }

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_SURVEILLANCE :

		prf_num = ele->surv_prf_num;
		break;

	    case ORPGVCP_DOPPLER1 :

		prf_num = ele->dop_prf_num_1;
		break;

	    case ORPGVCP_DOPPLER2 :

		prf_num = ele->dop_prf_num_2;
		break;

	    case ORPGVCP_DOPPLER3 :

		prf_num = ele->dop_prf_num_3;
		break;

	    default :

		prf_num = -1;
		break;

	}

	return (prf_num);

/* End of ORPGVCP_get_prf_num() */
}

/************************************************************************
 *	Description: This function sets the PRF number	 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num -  VCP number					*
 *		cut     -  elevation cut number				*
 *		prf_type - ORPGVCP_SURVEILLANCE				*
 *			   ORPGVCP_DOPPLER1				*
 *			   ORPGVCP_DOPPLER2				*
 *			   ORPGVCP_DOPPLER3				*
 *		prf_num -  PRF number (1-8)				*
 *	Output: NONE							*
 *	Return: config on success, -1 on error				*
 ************************************************************************/
int ORPGVCP_set_prf_num( int vcp_num, int cut, int type, int prf_num ){

	int	indx;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

	if ((type != ORPGVCP_SURVEILLANCE) &&
	    (type != ORPGVCP_DOPPLER1) &&
	    (type != ORPGVCP_DOPPLER2) &&
	    (type != ORPGVCP_DOPPLER3)) {

	    return (-1);

	}

	if ((prf_num < 0) || (prf_num > PRFMAX)) {

	    return (-1);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_SURVEILLANCE :

		ele->surv_prf_num = prf_num;
		break;

	    case ORPGVCP_DOPPLER1 :

		ele->dop_prf_num_1 = prf_num;
		break;

	    case ORPGVCP_DOPPLER2 :

		ele->dop_prf_num_2 = prf_num;
		break;

	    case ORPGVCP_DOPPLER3 :

		ele->dop_prf_num_3 = prf_num;
		break;

	    default :

		prf_num = -1;
		break;

	}

	return (prf_num);

/* End of ORPGVCP_set_prf_num() */
}

/************************************************************************
 *	Description: This function returns the pulse count per radial	*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		type    - ORPGVCP_SURVEILLANCE				*
 *			  ORPGVCP_DOPPLER1				*
 *			  ORPGVCP_DOPPLER2				*
 *			  ORPGVCP_DOPPLER3				*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: pulse count on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_pulse_count( int vcp_num, int cut, int type, ... ){

	int	indx;
	int	pulse_count;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_elevation_cut_data_t *rda_ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*	Validate the prf_type. 					*/

	if ((type != ORPGVCP_SURVEILLANCE) &&
	    (type != ORPGVCP_DOPPLER1) &&
	    (type != ORPGVCP_DOPPLER2) &&
	    (type != ORPGVCP_DOPPLER3)) {

	    return (-1);

	}

/*      If the data has changed in the file then we need 	*
	to re-read it.                                          */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated)
	    ORPGVCP_read ();

/*	Validate cut number is within range.			*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, type );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/*	Extract pulse count.						*/

	    rda_ele = (VCP_elevation_cut_data_t *) &rda_vcp->vcp_elev_data.data[cut];

	    switch (type) {

	   	 case ORPGVCP_SURVEILLANCE :

		    pulse_count = rda_ele->surv_prf_pulse;
		    break;

	    	case ORPGVCP_DOPPLER1 :

		    pulse_count = rda_ele->dopp_prf_pulse1;
		    break;

	    	case ORPGVCP_DOPPLER2 :

		    pulse_count = rda_ele->dopp_prf_pulse2;
		    break;

	    	case ORPGVCP_DOPPLER3 :

		    pulse_count = rda_ele->dopp_prf_pulse3;
		    break;

	    	default :

		    pulse_count = -1;
		    break;

	    }

	    return (pulse_count);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);


	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_SURVEILLANCE :

		pulse_count = ele->surv_pulse_cnt;
		break;

	    case ORPGVCP_DOPPLER1 :

		pulse_count = ele->pulse_cnt_1;
		break;

	    case ORPGVCP_DOPPLER2 :

		pulse_count = ele->pulse_cnt_2;
		break;

	    case ORPGVCP_DOPPLER3 :

		pulse_count = ele->pulse_cnt_3;
		break;

	    default :

		pulse_count = -1;
		break;

	}

	return (pulse_count);

/* End of ORPGVCP_get_pulse_count()*/
}

/************************************************************************
 *	Description: This function sets the pulse count per radial	*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		type    - ORPGVCP_SURVEILLANCE				*
 *			  ORPGVCP_DOPPLER1				*
 *			  ORPGVCP_DOPPLER2				*
 *			  ORPGVCP_DOPPLER3				*
 *		count   - pulse count					*
 *	Output: NONE							*
 *	Return: pulse count on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_pulse_count( int vcp_num, int cut, int type, int count ){

	int	indx;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

	if ((type != ORPGVCP_SURVEILLANCE) &&
	    (type != ORPGVCP_DOPPLER1) &&
	    (type != ORPGVCP_DOPPLER2) &&
	    (type != ORPGVCP_DOPPLER3)) {

	    return (-1);

	}

	if ((count < 0) || (count > ORPGVCP_PULSE_COUNT_MAX)) {

	    return (-1);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_SURVEILLANCE :

		ele->surv_pulse_cnt = count;
		break;

	    case ORPGVCP_DOPPLER1 :

		ele->pulse_cnt_1 = count;
		break;

	    case ORPGVCP_DOPPLER2 :

		ele->pulse_cnt_2 = count;
		break;

	    case ORPGVCP_DOPPLER3 :

		ele->pulse_cnt_3 = count;
		break;

	    default :

		count = -1;
		break;

	}

	return (count);

/* End of ORPGVCP_set_pulse_count() */
}

/************************************************************************
 *	Description: This function returns the edge angle (deg)		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		type    - ORPGVCP_DOPPLER1				*
 *			  ORPGVCP_DOPPLER2				*
 *			  ORPGVCP_DOPPLER3				*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: azimuth angle (deg) on success, -99.9 on error		*
 ************************************************************************/
float ORPGVCP_get_edge_angle( int vcp_num, int cut, int type, ... ){

	int	indx;
	float	angle;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_elevation_cut_data_t *rda_ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*	Validate the prf_type. 					*/

	if ((type != ORPGVCP_DOPPLER1) &&
	    (type != ORPGVCP_DOPPLER2) &&
	    (type != ORPGVCP_DOPPLER3)) {

	    return (-99.9);

	}

/*      If the data has changed in the file then we need 	*
	to re-read it.                                          */

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated)
	    ORPGVCP_read ();

/*	Validate cut number is within range.			*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-99.9);

	}

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, type );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-99.9);

/* 	Extract edge angle.						*/

	    rda_ele = (VCP_elevation_cut_data_t *) &rda_vcp->vcp_elev_data.data[cut];

	    switch (type) {

	    	case ORPGVCP_DOPPLER1 :

		    angle = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, rda_ele->edge_angle1 );
		    break;

	    	case ORPGVCP_DOPPLER2 :

		    angle = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, rda_ele->edge_angle2 );
		    break;

	    	case ORPGVCP_DOPPLER3 :

		    angle = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, rda_ele->edge_angle3 );
		    break;

	    	default :

		    angle = -99.9;
		    break;

	    }

	    return (angle);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-99.9);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	if ((cut < 0) || (indx >= ECUTMAX)) {

	    return (-99.9);

	}

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_DOPPLER1 :

		angle = ele->azi_ang_1/10.0;
		break;

	    case ORPGVCP_DOPPLER2 :

		angle = ele->azi_ang_2/10.0;
		break;

	    case ORPGVCP_DOPPLER3 :

		angle = ele->azi_ang_3/10.0;
		break;

	    default :

		angle = -99.9;
		break;

	}

	return (angle);

/* End of ORPGVCP_get_edge_angle() */
}

/************************************************************************
 *	Description: This function sets the azimuth edge angle		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		type    - ORPGVCP_DOPPLER1				*
 *			  ORPGVCP_DOPPLER2				*
 *			  ORPGVCP_DOPPLER3				*
 *		angle   - edge azimuth angle (0 - 359.9)		*
 *	Output: NONE							*
 *	Return: angle on success, -99.9 on error			*
 ************************************************************************/
float ORPGVCP_set_edge_angle( int vcp_num, int cut, int type, float angle ){

	int	indx;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

	if ((type != ORPGVCP_DOPPLER1) &&
	    (type != ORPGVCP_DOPPLER2) &&
	    (type != ORPGVCP_DOPPLER3)) {

	    return (-99.9);

	}

	if ((angle < 0.0) || (angle > 359.9)) {

	    return (-99.9);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-99.9);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-99.9);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_DOPPLER1 :

		ele->azi_ang_1 = (short) (angle*10);
		break;

	    case ORPGVCP_DOPPLER2 :

		ele->azi_ang_2 = (short) (angle*10);
		break;

	    case ORPGVCP_DOPPLER3 :

		ele->azi_ang_3 = (short) (angle*10);
		break;

	    default :

		angle = -99.9;
		break;

	}

	return (angle);

/* End of ORPGVCP_set_edge_angle() */
}

/************************************************************************
 *	Description: This function returns the data SNR threshold	*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		type    - ORPGVCP_REFLECTIVITY				*
 *			  ORPGVCP_VELOCITY				*
 *			  ORPGVCP_SPECTRUM_WIDTH			*
 *			  ORPGVCP_DIFFERENTIAL_Z			*
 *			  ORPGVCP_CORRELATION_COEF			*
 *			  ORPGVCP_DIFFERENTIAL_PHASE			*
 *		vol_num - volume scan number				*
 *	Output: NONE							*
 *	Return: SNR threshold (-99.9 on error)				*
 ************************************************************************/
float ORPGVCP_get_threshold ( int vcp_num, int cut, int type, ... ){

	int	indx;
	float	threshold;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_elevation_cut_data_t *rda_ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

	if ((type != ORPGVCP_REFLECTIVITY) &&
	    (type != ORPGVCP_VELOCITY) &&
	    (type != ORPGVCP_SPECTRUM_WIDTH) &&
            (type != ORPGVCP_DIFFERENTIAL_Z) &&
            (type != ORPGVCP_CORRELATION_COEF) &&
            (type != ORPGVCP_DIFFERENTIAL_PHASE) ){

	    return (-99.9);

	}

/* 	Validate cut number is within range.			*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-99.9);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, type );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-99.9);

/*	Extract threshold.						*/

	    rda_ele = (VCP_elevation_cut_data_t *) &rda_vcp->vcp_elev_data.data[cut];

	    switch (type) {

	    	case ORPGVCP_REFLECTIVITY :

		    threshold = ((float) rda_ele->refl_thresh)/8.0;
		    break;

	    	case ORPGVCP_VELOCITY :

		    threshold = ((float) rda_ele->vel_thresh)/8.0;
		    break;

	    	case ORPGVCP_SPECTRUM_WIDTH :

		    threshold = ((float) rda_ele->sw_thresh)/8.0;
		    break;

	    	case ORPGVCP_DIFFERENTIAL_Z :

		    threshold = ((float) rda_ele->diff_refl_thresh)/8.0;
		    break;

	    	case ORPGVCP_CORRELATION_COEF :

		    threshold = ((float) rda_ele->corr_coeff_thresh)/8.0;
		    break;

	    	case ORPGVCP_DIFFERENTIAL_PHASE :

		    threshold = ((float) rda_ele->diff_phase_thresh)/8.0;
		    break;

	    	default :

		    threshold = -99.9;
		    break;

	    }

	    return (threshold);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-99.9);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_REFLECTIVITY :

		threshold = ((float) ele->surv_thr_parm)/8.0;
		break;

	    case ORPGVCP_VELOCITY :

		threshold = ((float) ele->vel_thrsh_parm)/8.0;
		break;

	    case ORPGVCP_SPECTRUM_WIDTH :

		threshold = ((float) ele->spw_thrsh_parm)/8.0;
		break;

	    case ORPGVCP_DIFFERENTIAL_Z :

		threshold = ((float) ele->zdr_thrsh_parm)/8.0;
		break;

	    case ORPGVCP_CORRELATION_COEF :

		threshold = ((float) ele->corr_thrsh_parm)/8.0;
		break;

	    case ORPGVCP_DIFFERENTIAL_PHASE :

		threshold = ((float) ele->phase_thrsh_parm)/8.0;
		break;

	    default :

		threshold = -99.9;
		break;

	}

	return (threshold);

/* End of ORPGVCP_get_threshold() */
}

/************************************************************************
 *	Description: This function sets the PRF number	 		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		type    - ORPGVCP_REFLECTIVITY				*
 *			  ORPGVCP_VELOCITY				*
 *			  ORPGVCP_SPECTRUM_WIDTH			*
 *			  ORPGVCP_DIFFERENTIAL_Z			*
 *			  ORPGVCP_CORRELATION_COEF			*
 *			  ORPGVCP_DIFFERENTIAL_PHASE			*
 *		threshold - -12.0 to 20.0 dB				*
 *	Output: NONE							*
 *	Return: threshold (-99.9 on error)				*
 ************************************************************************/
float ORPGVCP_set_threshold( int vcp_num, int cut, int type, float threshold ){

	int	indx;
	short	num;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

	if ((type != ORPGVCP_REFLECTIVITY) &&
	    (type != ORPGVCP_VELOCITY) &&
	    (type != ORPGVCP_SPECTRUM_WIDTH) &&
            (type != ORPGVCP_DIFFERENTIAL_Z) &&
            (type != ORPGVCP_CORRELATION_COEF) &&
            (type != ORPGVCP_DIFFERENTIAL_PHASE) ){

	    return (-99.9);

	}

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-99.9);

	}

	if ((threshold < -12.0) ||
	    (threshold >  20.0)) {

	    return (-99.9);

	}

/*	Scale the threshold values from float into a short scaled by	*
 *	8.								*/

	if (threshold > 0.0) {

	    num = (short) (threshold * 8 + 0.5);

	} else {

	    num = (short) (threshold * 8 - 0.5);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-99.9);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	switch (type) {

	    case ORPGVCP_REFLECTIVITY :

		ele->surv_thr_parm = num;
		break;

	    case ORPGVCP_VELOCITY :

		ele->vel_thrsh_parm = num;
		break;

	    case ORPGVCP_SPECTRUM_WIDTH :

		ele->spw_thrsh_parm = num;
		break;

	    case ORPGVCP_DIFFERENTIAL_Z :

		ele->zdr_thrsh_parm = num;
		break;

	    case ORPGVCP_CORRELATION_COEF :

		ele->corr_thrsh_parm = num;
		break;

	    case ORPGVCP_DIFFERENTIAL_PHASE :

		ele->phase_thrsh_parm = num;
		break;

	    default :

		threshold = -99.9;
		break;

	}

	return (threshold);

/* End of ORPGVCP_set_threshold() */
}

/************************************************************************
 *	Description: This function returns the scan azimuth rate	*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		vol_num	- volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: azimuth rate (deg/sec) on siccess, -1 0n error		*
 ************************************************************************/
float ORPGVCP_get_azimuth_rate ( int vcp_num, int cut, ... ){

	int	indx;
	float	rate;
	Vcp_struct	*vcp;
	Ele_attr	*ele;
	VCP_elevation_cut_data_t *rda_ele;
	VCP_ICD_msg_t 	*rda_vcp;

        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num = 0;
        unsigned int vol_num = 0;

        va_list ap;

/*	Validate cut number is within range. 			*/

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1.0);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

        if (rdavcp_flag && !Rda_vcp_updated )
            ORPGVCP_read_rdavcp ();

	else if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*      If RDA VCP, then .....                                          */

        if( rdavcp_flag ){

/*      Set up for volume scan number as optional argument.             */

            if( volnum_flag ){

                va_start( ap, cut );
                vol_num = va_arg( ap, unsigned int );
                vol_num %= 2;
		va_end( ap );

            } else {

                vol_num = (unsigned int) Rda_vcp.last_entry;
            }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/* 	Extract azimuth rate.						*/

	    rda_ele = (VCP_elevation_cut_data_t *) &rda_vcp->vcp_elev_data.data[cut];

	    rate = rda_ele->azimuth_rate * ORPGVCP_AZIMUTH_RATE_FACTOR;
	    return (rate);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1.0);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	rate = ele->azi_rate * ORPGVCP_AZIMUTH_RATE_FACTOR;

	return (rate);

/* End of ORPGVCP_get_azimuth_rate() */
}

/************************************************************************
 *	Description: This function sets the scan azimuth rate		*
 *		     associated with a selected VCP and elevation cut.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		cut     - elevation cut number				*
 *		rate    - 0.0 to 30.0 deg/sec (5rpm max)		*
 *	Output: NONE							*
 *	Return: rate (-1.0 on error)					*
 ************************************************************************/
float ORPGVCP_set_azimuth_rate( int vcp_num, int cut, float rate ){

	int	indx;
	int	num;
	Vcp_struct	*vcp;
	Ele_attr	*ele;

	if ((cut < 0) || (cut >= ECUTMAX)) {

	    return (-1.0);

	}

	if ((rate < 2.0) &&
	    (rate > 32.0)) {

	    return (-1.0);

	}

/*	Scale the azimuth rate values from float into a scaled short.	*/
	num = ORPGVCP_rate_degs_to_BAMS( rate );

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1.0);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	ele = (Ele_attr *) &vcp->vcp_ele [cut];

	ele->azi_rate = num;

	return (rate);

/* End of ORPGVCP_set_azimuth_rate() */
}

/************************************************************************
 *	Description: This function returns a pointer to the start of	*
 *		     the allowable PRF table for the specified VCP	*
 *		     (referenced by position).				*
 *									*
 *	Input:  pos - position of VCP in VCP adaptation data table.	*
 *	Output: NONE							*
 *	Return: pointer to allowable PRF data for VCP, NULL on error	*
 ************************************************************************/
short* ORPGVCP_allowable_prf_ptr( int pos ){

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return ((short *) NULL);

	}

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

	return	(short *) &Vcp_adapt.alwblprf [pos][0];

/* End of ORPGVCP_allowable_prf_ptr() */
}

/************************************************************************
 *	Description: This function returns the VCP number from the	*
 *		     specified allowable PRF table.			*
 *									*
 *	Input:  pos - position of VCP in VCP adaptation data table.	*
 *	Output: NONE							*
 *	Return: VCP number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_allowable_prf_vcp_num( int pos ){

	Vcp_alwblprf_t	*alwblprf;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	return alwblprf->vcp_num;

/* End of ORPGVCP_get_allowable_prf_vcp_num() */
}

/************************************************************************
 *	Description: This function sets the VCP number from the		*
 *		     specified allowable PRF table.			*
 *									*
 *	Input:  pos - position of VCP in VCP adaptation data table.	*
 *		vcp - VCP number					*
 *	Output: NONE							*
 *	Return: VCP number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_allowable_prf_vcp_num( int pos, int vcp ){

	Vcp_alwblprf_t	*alwblprf;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

	if ((vcp < 1) || (vcp > 32768)) {

	    return (-1);

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	alwblprf->vcp_num = (short) vcp;

	return (vcp);

/* End of ORPGVCP_set_allowable_prf_vcp_num() */
}

/************************************************************************
 *	Description: This function returns the number of allowable PRFs	*
 *		     for a specified allowable PRF table VCP.		*
 *									*
 *	Input:  vcp_num - VCP number.					*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: number of allowable PRFs on success, -1 on error	*
 ************************************************************************/
int ORPGVCP_get_allowable_prfs( int vcp_num, ... ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

/*	The number of allowable PRFs is not affected by whether it is	*
	an RDA or RPG VCP.						*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*	Validate the input arguments.  If out of range return -1.	*/

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

	if (pos >= VCPMAX) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	return alwblprf->num_alwbl_prf;

/* End of ORPGVCP_get_allowable_prfs() */
}

/************************************************************************
 *	Description: This function sets the number of allowable PRFs	*
 *		     for a specified allowable PRF table VCP.		*
 *									*
 *	Input:  vcp_num - VCP number.					*
 *		num - number of allowable PRFs				*
 *	Output: NONE							*
 *	Return: number of allowable PRFs on success, -1 on error	*
 ************************************************************************/
int ORPGVCP_set_allowable_prfs( int vcp_num, int num ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

/*	Validate the input arguments.  If out of range return -1.	*/

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

	if (pos >= VCPMAX) {

	    return (-1);

	}

	if ((num < 0) || (num > PRFMAX)) {

	    return (-1);

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	alwblprf->num_alwbl_prf = (short) num;

	return (num);

/* End of ORPGVCP_set_allowable_prfs() */
}

/************************************************************************
 *	Description: This function returns the PRF number of the	*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  vcp_num - VCP number.					*
 *		indx - allowable PRF index (0 < X < num PRFs)		*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: PRF number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_allowable_prf( int vcp_num, int	indx, ... ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

/*	The allowable PRF is not affected by whether it is an RDA 
	or RPG VCP.							*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

/*	Validate the input arguments.  If out of range return -1.	*/

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

	if (pos >= VCPMAX) {

	    return (-1);

	}

	if ((indx < 0) || (indx >= PRFMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	return alwblprf->prf_num [indx];

/* End of ORPGVCP_get_allowable_prf() */
}

/************************************************************************
 *	Description: This function sets the PRF number of the		*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  pos  - VCP number.					*
 *		indx - allowable PRF index (0 < X < num PRFs)		*
 *		num  - PRF number					*
 *	Output: NONE							*
 *	Return: PRF number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_allowable_prf( int vcp_num, int indx, int num ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

/*	Validate the input arguments.  If out of range return -1.	*/

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

	if (pos >= VCPMAX) {

	    return (-1);

	}

	if ((indx < 0) || (indx >= PRFMAX)) {

	    return (-1);

	}

	if ((num < 0) || (num > PRFMAX)) {

	    return (-1);

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	alwblprf->prf_num [indx] = (short) num;

	return (num);

/* End of ORPGVCP_set_allowable_prf() */
}

/************************************************************************
 *	Description: This function gets the default PRF of the		*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  vcp_num  - VCP number					*
 *		elev_num - elevation cut number				*
 *		vol_num - volume scan number				*
 *	Output: NONE							*
 *	Return: PRF number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_allowable_prf_default( int vcp_num, int elev_num, ... ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

/*	The default PRF is not affected by whether it is an RDA 	*
	or RPG VCP.							*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

	if ((elev_num < 0) || (elev_num >= ECUTMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	return (int) alwblprf->pulse_cnt [elev_num][PRFMAX];

/* End of ORPGVCP_get_allowable_prf_default() */
}

/************************************************************************
 *	Description: This function sets the default PRF of the		*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  vcp_num  - VCP number					*
 *		elev_num - elevation cut number				*
 *		prf      - PRF number					*
 *	Output: NONE							*
 *	Return: PRF number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_allowable_prf_default( int vcp_num, int elev_num, int prf ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

	if ((elev_num < 0) || (elev_num >= ECUTMAX)) {

	    return (-1);

	}

	if ((prf < 0) || (prf > PRFMAX)) {

	    return (-1);

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	alwblprf->pulse_cnt [elev_num][PRFMAX] = (short) prf;

	return (prf);

/* End of ORPGVCP_set_allowable_prf_default() */
}

/************************************************************************
 *	Description: This function gets the pulse count of the		*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  vcp_num  - VCP number					*
 *		elev_num - elevation cut number				*
 *		prf      - PRF number					*
 *		vol_num	 - volume scan number.				*
 *	Output: NONE							*
 *	Return: pulse count on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_allowable_prf_pulse_count ( int vcp_num, int elev_num, int prf, ... ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

/*	The pulse count is not affected by whether it is an RDA 	*
	or RPG VCP.							*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

	if ((elev_num < 0) || (elev_num >= ECUTMAX)) {

	    return (-1);

	}

	if ((prf < 1) || (prf > PRFMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	return (int) alwblprf->pulse_cnt [elev_num][prf-1];

/* End of ORPGVCP_get_allowable_prf_pulse_count () */
}

/************************************************************************
 *	Description: This function sets the pulse count of the		*
 *		     specified entry in the allowable PRF table.	*
 *									*
 *	Input:  vcp_num  - VCP number					*
 *		elev_num - elevation cut number				*
 *		prf      - PRF number					*
 *		count    - pulse count					*
 *	Output: NONE							*
 *	Return: pulse count on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_allowable_prf_pulse_count( int vcp_num, int elev_num,
                                           int prf, int count ){

	int	pos;
	Vcp_alwblprf_t	*alwblprf;

	for (pos=0;pos<VCPMAX; pos++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (pos) == vcp_num) {

		break;
	    }
	}

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

	if ((elev_num < 0) || (elev_num >= ECUTMAX)) {

	    return (-1);

	}

	if ((prf < 1) || (prf > PRFMAX)) {

	    return (-1);

	}

	if ((count < 0) || (count > 32768)) {

	    return (-1);

	}

	alwblprf = (Vcp_alwblprf_t *) ORPGVCP_allowable_prf_ptr (pos);

	alwblprf->pulse_cnt [elev_num][prf-1] = (short) count;

	return (count);

/* End of ORPGVCP_set_allowable_prf_pulse_count() */
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
float* ORPGVCP_prf_ptr (){

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	return	(float *) &Vcp_adapt.prfvalue [0];

/* End of ORPGVCP_prf_ptr () */
}

/************************************************************************
 *	Description: This function returns the value of the specified	*
 *		     PRF.						*
 *									*
 *	Input:  prf - PRF number					*
 *	Output: NONE							*
 *	Return: PRF value (Hz) on success, -1 on error			*
 ************************************************************************/
float ORPGVCP_get_prf_value ( int prf ){


/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}

	if ((prf < 1) || (prf > 8)) {

	    return (-1.0);

	}

	return	(Vcp_adapt.prfvalue [prf-1]);

/* End of ORPGVCP_get_prf_value () */
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
short* ORPGVCP_vcp_times_ptr (){

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	return (short *) &Vcp_adapt.vcp_times [0];

/* End of ORPGVCP_vcp_times_ptr () */
}

/************************************************************************
 *      Description: This function returns a pointer to the start of    *
 *                   the VCP flags table in the RDA adaptation data 	*
 *                   block.                                             *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: pointer to VCP flags                               *
 ************************************************************************/
short* ORPGVCP_vcp_flags_ptr (){

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated) {

            ORPGVCP_read ();

        }
        return (short *) &Vcp_adapt.vcp_flags [0];

/* End of ORPGVCP_vcp_flags_ptr () */
}


/************************************************************************
 *	Description: This function gets the volume execution times	*
 *		     for a specitifed VCP in the VCP times table.	*
 *									*
 *	Input:  vcp_num - VCP number					*
 *		vol_num	- volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: volume execution time (seconds) on success, -1 on error	*
 ************************************************************************/
int ORPGVCP_get_vcp_time( int vcp_num, ... ){

	int	indx;
	short	*vcp_times;

/*	The VCP time is for non-SAILS VCPS only.			*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

	indx = ORPGVCP_index (vcp_num);

	if (indx < 0) {

	    return (-1);

	}

	vcp_times = ORPGVCP_vcp_times_ptr ();

	return (vcp_times [indx]);

/* End of ORPGVCP_get_vcp_time() */
}


/************************************************************************
 *      Description: This function gets the VCP flags for a specified   *
 *                   VCP in the VCP flags table.       			*
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *		vol_num - volume scan number (optional)			*
 *      Output: NONE                                                    *
 *      Return: VCP flags on success, 0 on error 			*
 ************************************************************************/
short ORPGVCP_get_vcp_flags( int vcp_num, ... ){

        int     indx;
        short   *vcp_flags;

/*	The VCP flags are for locally defined VCPS only.	*/ 

	vcp_num &= ORPGVCP_CLEAR_FLAGS;
        indx = ORPGVCP_index (vcp_num);

        if (indx < 0) 
            return (0);

        vcp_flags = ORPGVCP_vcp_flags_ptr ();

        return (vcp_flags [indx]);

/* End of ORPGVCP_get_vcp_flags() */
}


/************************************************************************
 *	Description: This function sets the volume execution times	*
 *		     for a specitifed VCP in the VCP times table.	*
 *									*
 *	Input:  vcp_num - VCP number					*
 *		vcp_time - execution time (seconds)			*
 *	Output: NONE							*
 *	Return: volume execution time (seconds) on success, -1 on error	*
 ************************************************************************/
int ORPGVCP_set_vcp_time( int vcp_num, int vcp_time ){

	int	indx;
	short	*vcp_times;

	if ((vcp_time < 0) || (vcp_time > 3600)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

	if (indx < 0) {

	    return (-1);

	}

	vcp_times = ORPGVCP_vcp_times_ptr ();

	vcp_times [indx] = vcp_time;

	return (vcp_time);

/* End of ORPGVCP_set_vcp_time() */
}

/************************************************************************
 *      Description: This function sets the VCP flags for a specified   *
 *                   VCP in the VCP flags table.                        *
 *                                                                      *
 *      Input:  vcp_num - VCP number                                    *
 *              vcp_flags - VCP flags (see rdacnt.h)                    *
 *      Output: NONE                                                    *
 *      Return: VCP flags on success, 0 on error 			*
 ************************************************************************/
short ORPGVCP_set_vcp_flags( int vcp_num, short vcp_flags ){

        int     indx;
        short   *vcp_flags_table;

        indx = ORPGVCP_index (vcp_num);

        if (indx < 0) 
            return (0);

        vcp_flags_table = ORPGVCP_vcp_flags_ptr ();

        vcp_flags_table [indx] = vcp_flags;

        return (vcp_flags);

/* End of ORPGVCP_set_vcp_flags() */
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
int* ORPGVCP_unambiguous_range_ptr (){

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	return (int *) &Vcp_adapt.unambigr [0][0];

/* End of ORPGVCP_unambiguous_range_ptr () */
}

/************************************************************************
 *      Description: This function returns a pointer to the start of    *
 *                   the unambiguous range table in the RDA adaptation  *
 *                   data blocki for a particular delta PRI.            *
 *                                                                      *
 *      Input:  delta PRI (assumed in range 1-5)                        *
 *      Output: NONE                                                    *
 *      Return: pointer to unambiguous range data                       *
 ************************************************************************/
int* ORPGVCP_unambiguous_range_table_ptr( int delta_pri ){


/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */

        if (!Vcp_adapt_updated) {

            ORPGVCP_read ();

        }
        return (int *) &Vcp_adapt.unambigr [delta_pri-1][0];

/* End of ORPGVCP_unambiguous_range_table_ptr() */
}

/************************************************************************
 *	Description: This function returns a pointer to the delta PRI 	*
 *		     value RDA adaptation				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: pointer to delta pri 					*
 ************************************************************************/
int* ORPGVCP_delta_pri_ptr (){


/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	return (int *) &Vcp_adapt.delta_pri;

/* End of ORPGVCP_delta_pri_ptr () */
}


/************************************************************************
 *	Description: This function tries to match the input VCP number	*
 *		     with an entry in the adaptation data.  If a match	*
 *		     is found, the position in the adaptation data is	*
 *		     returned.  If not, then a -1 is returned.		*
 *									*
 *	Input:  vcp_number - VCP number					*
 *	Output: NONE							*
 *	Return: position of VCP in VCP table. -1 is returned for no	*
 *		match							*
 ************************************************************************/
int ORPGVCP_index( int vcp_number ){

	int		i;

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}

	for (i=0;i<VCPMAX;i++) {

	    if (vcp_number == ORPGVCP_get_vcp_num (i)) {

		return i;

	    }
	}

	return -1;

/* End of ORPGVCP_index() */
}

/************************************************************************
 *	Description: This function returns the VCP number for a		*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	pos - VCP position in table				*
 *	Output: NONE							*
 *	Return: VCP number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_vcp_num( int pos ){

	Vcp_struct	*vcp;

/*	Validate the input arguments.  If out of range return	*
 *	-1.							*/

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (pos);

	return (int) vcp->vcp_num;

/* End of ORPGVCP_get_vcp_num() */
}

/************************************************************************
 *	Description: This function sets the VCP number for a		*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	pos - VCP position in table				*
 *		vcp_num - VCP number					*
 *	Output: NONE							*
 *	Return: VCP number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_vcp_num( int pos, int vcp_num ){

	Vcp_struct	*vcp;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((vcp_num < 1) || (vcp_num > 32768)) {

	    return (-1);

	}

	if ((pos < 0) || (pos >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (pos);
	vcp->vcp_num = (short) vcp_num;

	return (vcp_num);

/* End of ORPGVCP_set_vcp_num() */
}

/************************************************************************
 *	Description: This function returns the pulse width for a	*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: pulse width on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_pulse_width( int vcp_num, ... ){

	int	indx;
	Vcp_struct	*vcp;

/*	The pulse width is not affected by RDA or RPG VCP.	*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	
	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	return (int) vcp->pulse_width;

/* End of ORPGVCP_get_pulse_width() */
}

/************************************************************************
 *	Description: This function sets the pulse width for a		*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input:  vcp_num - VCP number					*
 *		width - pulse width (2: short or 4: long)		*
 *	Output: NONE							*
 *	Return: pulse width on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_pulse_width( int vcp_num, int width ){

	int	indx;
	Vcp_struct	*vcp;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((width != ORPGVCP_SHORT_PULSE) &&
	    (width != ORPGVCP_LONG_PULSE)) {

	    return (-1);

	}


	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	vcp->pulse_width = (unsigned char) width;

	return (width);

/* End of ORPGVCP_set_pulse_width() */
}

/************************************************************************
 *	Description: This function returns the clutter map # for a	*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		vol_num	- volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: map number on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_get_clutter_map_num( int vcp_num, ... ){

	int	indx;
	Vcp_struct	*vcp;

/*	The clutter map is not affected by RDA or RPG VCP.	*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	
	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	return (int) vcp->clutter_map_num;

/* End of ORPGVCP_get_clutter_map_num() */
}

/************************************************************************
 *	Description: This function sets the clutter map # for a		*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		num - map # (1 or 2)					*
 *	Output: NONE							*
 *	Return: clutter map number on success, -1 on error		*
 ************************************************************************/
int ORPGVCP_set_clutter_map_num( int vcp_num, int num ){

	int	indx;
	Vcp_struct	*vcp;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((num != 1) &&
	    (num != 2)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	vcp->clutter_map_num = (short) num;

	return (num);

/* End of ORPGVCP_set_clutter_map_num() */
}

/************************************************************************
 *	Description: This function returns the # of elevations for a	*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: number of elevations on success, -1 on error		*
 ************************************************************************/
int ORPGVCP_get_num_elevations( int vcp_num, ... ){

	int	indx;
	Vcp_struct	*vcp;
	VCP_ICD_msg_t	*rda_vcp;
        int     rdavcp_flag = vcp_num & ORPGVCP_RDAVCP;
        int     volnum_flag = vcp_num & ORPGVCP_VOLNUM;
	int	rda_vcp_num;
	unsigned int vol_num = 0;

	va_list ap;

/*	If the data has changed in the file then we need to re-read	*
 *	it.								*/

        if (rdavcp_flag && !Rda_vcp_updated )
	    ORPGVCP_read_rdavcp ();

/*	Clear the flags in vcp_num if set. 				*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;
           
/*	If RDA VCP, then ..... 						*/

	if( rdavcp_flag ){

/*	Set up for volume scan number as optional argument.		*/

	    if( volnum_flag ){

		va_start( ap, vcp_num );
		vol_num = va_arg( ap, unsigned int );
		vol_num %= 2;
		va_end( ap );

	    } else {

		vol_num = (unsigned int) Rda_vcp.last_entry;

	    }

/*	Ensure vcp_num matches RDA VCP number in execution.		*/

	    rda_vcp = (VCP_ICD_msg_t *) &Rda_vcp.data[vol_num].rdcvcpta[0];
            rda_vcp_num = rda_vcp->vcp_msg_hdr.pattern_number;

            if( rda_vcp_num != vcp_num )
               return (-1);

/* 	Extract the number of elevation cuts.				*/

	    return (rda_vcp->vcp_elev_data.number_cuts);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

	if (!Vcp_adapt_updated) 
	    ORPGVCP_read ();

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	return (int) vcp->n_ele;

/* End of ORPGVCP_get_num_elevations() */
}

/************************************************************************
 *	Description: This function sets the # of elevations for a	*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		num - number of elevations				*
 *	Output: NONE							*
 *	Return: number of elevations on success, -1 on error		*
 ************************************************************************/
int ORPGVCP_set_num_elevations( int vcp_num, int num ){

	int	indx;
	Vcp_struct	*vcp;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((num < 0) &&
	    (num >ECUTMAX)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	vcp->n_ele = (short) num;
        vcp->msg_size = VCP_ATTR_SIZE + num*ELE_ATTR_SIZE;

	return (num);

/* End of ORPGVCP_set_num_elevations() */
}

/************************************************************************
 *	Description: This function returns the pattern type for a	*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: ORPGVCP_PATTERN_TYPE_CONSTANT,				*
 *		ORPGVCP_PATTERN_TYPE_HORIZONTAL,			*
 *		ORPGVCP_PATTERN_TYPE_VERTICAL,				*
 *		ORPGVCP_PATTERN_TYPE_SEARCHLIGHT, or			*
 *		-1 on error						*
 ************************************************************************/
int ORPGVCP_get_pattern_type( int vcp_num, ... ){

	int	indx;
	Vcp_struct	*vcp;

/*	The pattern type is not affected by RDA or RPG VCP.	*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	
	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	return (int) vcp->type;

/* End of ORPGVCP_get_pattern_type() */
}

/************************************************************************
 *	Description: This function sets the VCP type for a		*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		type - VCP type: ORPGVCP_PATTERN_TYPE_CONSTANT		*
 *				 ORPGVCP_PATTERN_TYPE_HORIZONTAL	*
 *				 ORPGVCP_PATTERN_TYPE_VERTICAL		*
 *				 ORPGVCP_PATTERN_TYPE_SEARCHLIGHT	*
 *	Output: NONE							*
 *	Return: pattern type on success, -1 on error			*
 ************************************************************************/
int ORPGVCP_set_pattern_type( int vcp_num, int type ){

	int	indx;
	Vcp_struct	*vcp;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((type != ORPGVCP_PATTERN_TYPE_CONSTANT) &&
	    (type != ORPGVCP_PATTERN_TYPE_HORIZONTAL) &&
	    (type != ORPGVCP_PATTERN_TYPE_VERTICAL) &&
	    (type != ORPGVCP_PATTERN_TYPE_SEARCHLIGHT)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	vcp->type = (short) type;

	return (type);

/* End of ORPGVCP_set_pattern_type() */
}

/************************************************************************
 *	Description: This function returns the velocity resolution for	*
 *		     a specified entry in the VCP adaptation data table.*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		vol_num - volume scan number (optional)			*
 *	Output: NONE							*
 *	Return: ORPGVCP_VEL_RESOLUTION_LOW,				*
 *		ORPGVCP_VEL_RESOLUTION_HIGH, or				*
 *		-1 on error.						*
 ************************************************************************/
int ORPGVCP_get_vel_resolution( int vcp_num, ... ){

	int	indx;
	Vcp_struct	*vcp;

/*	The velocity resolution is not affected by RDA or RPG VCP.	*/

	vcp_num &= ORPGVCP_CLEAR_FLAGS;

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

/*	If the data has changed in the file then we need to re	*
 *	read it.						*/

	if (!Vcp_adapt_updated) {

	    ORPGVCP_read ();

	}
	
	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	return (int) vcp->vel_resolution;

/* End of ORPGVCP_get_vel_resolution() */
}

/************************************************************************
 *	Description: This function sets the velocity resolution for a	*
 *		     specified entry in the VCP adaptation data table.	*
 *									*
 *	Input: 	vcp_num - VCP number					*
 *		res - velocity resolution: 				*
 *				 ORPGVCP_VEL_RESOLUTION_HIGH		*
 *				 ORPGVCP_VEL_RESOLUTION_LOW		*
 *	Output: NONE							*
 *	Return: coded velocity resolution or -1 on error		*
 ************************************************************************/
int ORPGVCP_set_vel_resolution( int vcp_num, int res ){

	int	indx;
	Vcp_struct	*vcp;

/*	Validate the input arguments.  If out of range return -1.	*/

	if ((res != ORPGVCP_VEL_RESOLUTION_LOW) &&
	    (res != ORPGVCP_VEL_RESOLUTION_HIGH)) {

	    return (-1);

	}

	indx = ORPGVCP_index (vcp_num);

/*	if the selected VCP is not found in the adaptation data	*
 *	then return a -1.					*/

	if ((indx < 0) || (indx >= VCPMAX)) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);
	vcp->vel_resolution = (unsigned char) res;

	return (res);

/* End of ORPGVCP_set_vel_resolution() */
}

/************************************************************************
 *	Description: This function returns the delta PRI value from the	*
 *		     RDA adaptation data block.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: delta PRI value						*
 ************************************************************************/
int ORPGVCP_get_delta_pri (){

        int ret, delta_pri;

        ret = ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_DELTA_PRI,
                                           (void *) &delta_pri );
        if( ret == 0 )
           return( delta_pri );

/*      If the data has changed in the file then we need to re-read     *
 *      it.                                                             */
        if (!Vcp_adapt_updated) {

            ORPGVCP_read ();

        }

        LE_send_msg( GL_INFO, "ORPGRDA_ADPT_get_data_value( ORPGRDA_ADPT_DELTA_PRI ) Failed\n" );
        LE_send_msg( GL_INFO, "--->Returning value from RDA Control Adaptation data: %d\n",
                     Vcp_adapt.delta_pri );

	return	Vcp_adapt.delta_pri;

/* End of ORPGVCP_get_delta_pri () */
}

/************************************************************************
 *	Description: This function gets the number of VCPs in the	*
 *		     defined in the VCP adaptation data table.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: number of VCPs in VCP adaptation data table		*
 ************************************************************************/
int ORPGVCP_get_num_vcps (){

	int	i;
	Vcp_struct	*vcp;

	for (i=0;i<VCPMAX;i++) {

	    vcp = (Vcp_struct *) ORPGVCP_ptr (i);

	    if (vcp->vcp_num <= 0) {

		break;

	    }
	}

	return (i);

/* End of ORPGVCP_get_num_vcps() */
}

/************************************************************************
 *	Description: This function deletes a VCP from the VCP data	*
 *		     adaptation data tables.				*
 *									*
 *	Input:  vcp_num - VCP number of deleted VCP.			*
 *	Output: NONE							*
 *	Return: 0 on success; < 0 on failure.				*
 ************************************************************************/
int ORPGVCP_delete ( int vcp_num ){

	int	status;
	int	i;
	int	j;
	int	indx;
	int	num_vcps;
	Vcp_struct	*vcp1;
	Vcp_struct	*vcp2;
	short	*prf1;
	short	*prf2;
        short   *rdccon1;
        short   *rdccon2;
        short   *vcp_times;
        short   *vcp_flags;

	num_vcps = ORPGVCP_get_num_vcps();

	if (num_vcps < 1) {

	    return (-1);

	}

/*	If the VCP is not defined then return an error code.	*/

	indx = ORPGVCP_index (vcp_num);

	if (indx < 0) {

	    return (-2);

	}

/*	First, remove the VCP data from the VCP table.  If the VCP	*
 *	is not the last one, move all trailing VCPs up one position	*
 *	and clear out the last position.				*/

	for (i=indx;i<num_vcps-1;i++) {

	   vcp1 = (Vcp_struct *) ORPGVCP_ptr (i);
	   vcp2 = (Vcp_struct *) ORPGVCP_ptr (i+1);
	   memcpy ((char *) vcp1, (char *) vcp2, sizeof (Vcp_struct));

	}

	vcp1 = (Vcp_struct *) ORPGVCP_ptr (num_vcps-1);
	memset ((char *) vcp1, 0, sizeof (Vcp_struct));

/*	Next, we need to remove the VCP from the weather mode table and	*
 *	move up all trailing VCPs.					*/

	for (i=0;i<num_vcps;i++) {

	    if (ORPGVCP_get_wxmode_vcp (CLEAR_AIR_MODE, i) == vcp_num) {

		for (j=i;j<num_vcps-1;j++) {

		    status = ORPGVCP_set_wxmode_vcp (CLEAR_AIR_MODE, j,
		    		ORPGVCP_get_wxmode_vcp (CLEAR_AIR_MODE, j+1));

		}

		status = ORPGVCP_set_wxmode_vcp (CLEAR_AIR_MODE, num_vcps-1, 0);
		break;

	    } else if (ORPGVCP_get_wxmode_vcp (PRECIPITATION_MODE, i) == vcp_num) {

		for (j=i;j<num_vcps-1;j++) {

		    status = ORPGVCP_set_wxmode_vcp (PRECIPITATION_MODE, j,
		    		ORPGVCP_get_wxmode_vcp (PRECIPITATION_MODE, j+1));

		}

		status = ORPGVCP_set_wxmode_vcp (PRECIPITATION_MODE, num_vcps-1, 0);
		break;

	    }
	}

/*	Next, we want to remove the VCP from the allowable PRF table	*
 *	and move up data for all trailing VCPs.				*/

	for (i=0;i<num_vcps-1;i++) {

	    if (ORPGVCP_get_allowable_prf_vcp_num (i) == vcp_num) {

               indx = i;
               break;

	    }

	}

	for (i=indx;i<num_vcps-1;i++) {

	    prf1 = (short *) ORPGVCP_allowable_prf_ptr (i);
	    prf2 = (short *) ORPGVCP_allowable_prf_ptr (i+1);

	    memcpy ((char *) prf1, (char *) prf2, MXALWPRF*2);

	}

	prf1 = (short *) ORPGVCP_allowable_prf_ptr (num_vcps-1);

	memset ((char *) prf1, 0, MXALWPRF*2);

/*	Next, we want to remove the VCP from the RPG elevation cut  	*
 *	table and move up data for all trailing VCPs.			*/

	for (i=indx;i<num_vcps-1;i++) {

	    rdccon1 = (short *) ORPGVCP_elev_indicies_ptr (i);
	    rdccon2 = (short *) ORPGVCP_elev_indicies_ptr (i+1);

	    memcpy ((char *) rdccon1, (char *) rdccon2, ECUTMAX*2);

	}

	rdccon1 = (short *) ORPGVCP_elev_indicies_ptr (num_vcps-1);

	memset ((char *) rdccon1, 0, ECUTMAX*2);

/* 	We want to remove the VCP from the where-define arrays.		*/

        for (i=0;i<20;i++) {

            if (ORPGVCP_get_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP, i ) == vcp_num ){

                ORPGVCP_set_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP, i, 0 );
                break;

            }

        }

        for (i=0;i<20;i++) {

            if (ORPGVCP_get_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP, i ) == vcp_num ){

                ORPGVCP_set_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP, i, 0 );
                break;

            }

        }

/*	We want to remove the VCP time from the VCP times   	 	*
 *	table and move up data for all trailing VCPs.			*/

        vcp_times = ORPGVCP_vcp_times_ptr ();
	for (i=indx;i<num_vcps-1;i++) 
	    vcp_times[i] = vcp_times[i+1];

	vcp_times[num_vcps-1] = 0;

/*	Lastly, we want to remove the VCP flags from the VCP flags  	*
 *	table and move up data for all trailing VCPs.			*/

        vcp_flags = ORPGVCP_vcp_flags_ptr ();
	for (i=indx;i<num_vcps-1;i++) 
	    vcp_flags[i] = vcp_flags[i+1];

	vcp_flags[num_vcps-1] = 0;

/*	Return to caller.						*/
	return (0);

/* End of ORPGVCP_delete() */
}

/************************************************************************
 *	Description: This function adds a new VCP to the VCP data	*
 *		     adaptation data tables.				*
 *		     NOTE: This function ONLY searches for an unused	*
 *		     entry in the VCP table, sets the VCP number for	*
 *		     the entry and returns the table index.  It is up	*
 *		     to the calling application to populate specific	*
 *		     data in the table.					*
 *									*
 *	Input:  vcp_num - VCP number of new VCP.			*
 *		wx_mode - Weather mode associated with this VCP.	*
 *		where_defined - ORPG and/or ORDA defined VCP.		*
 *	Output: NONE							*
 *	Return: index in VCP adaptation data table of new VCP data	*
 *		on success, -1 on error.				*
 *      Note:  If the most significant bit of where_defined is set, 	*
 *             this is an experimental VCP.				*
 ************************************************************************/
int ORPGVCP_add( int vcp_num, int wx_mode, int where_defined ){

	int	i;
	int	j;
	int	indx;
	int	vcp_set;
	Vcp_struct	*vcp;

	indx = ORPGVCP_get_num_vcps();

/*	If we have exceeded the maximum number of VCPs allowed return	*
 *	-1.								*/

	if (indx >= VCPMAX) {

	    return (-1);

	}

/*	If the new VCP is already defined then return an error code.	*/

	if (ORPGVCP_index (vcp_num) >= 0) {

	    return (-1);

	}

	vcp = (Vcp_struct *) ORPGVCP_ptr (indx);

	memset (vcp,0,sizeof (Vcp_struct));

/*	Set some default properties for the new VCP.			*/

	ORPGVCP_set_vcp_num         (indx, vcp_num);

	ORPGVCP_set_num_elevations  (vcp_num, 1);
	ORPGVCP_set_pulse_width     (vcp_num, ORPGVCP_SHORT_PULSE);
	ORPGVCP_set_clutter_map_num (vcp_num, 1);
	ORPGVCP_set_pattern_type    (vcp_num, ORPGVCP_PATTERN_TYPE_CONSTANT);
	ORPGVCP_set_vel_resolution  (vcp_num, ORPGVCP_VEL_RESOLUTION_HIGH);

/*	By default allow all PRFs in the allowable PRF table.		*/

	ORPGVCP_set_allowable_prf_vcp_num (indx, vcp_num);
	ORPGVCP_set_allowable_prfs  (vcp_num, PRFMAX);

	for (i=0;i<PRFMAX;i++) {

	    ORPGVCP_set_allowable_prf  (vcp_num, i, i+1);

	}

/*	Zero out all pulse count entries in the elevation table.	*/

	for (i=0;i<ECUTMAX;i++) {
	    for (j=0;j<PRFMAX;j++) {

		ORPGVCP_set_allowable_prf_pulse_count  (vcp_num, i, j, 0);

	    }

/*	    Set the default PRF number to 0.				*/

	    ORPGVCP_set_allowable_prf_default (vcp_num, i, 0);

	}

/*	Zero out the volume time table entry.				*/

	ORPGVCP_set_vcp_time (vcp_num, 0);

/*	Put the new VCP in the wx mode table.  Check to make sure the	*
 *	VCP number doesn't already exist in the table.			*/

	if (wx_mode == CLEAR_AIR_MODE)  {

/*	The VCP already exists for the other weather mode so we	*
 *	need to remove it.					*/

	    for (i=0;i<20;i++) {

		if (ORPGVCP_get_wxmode_vcp (PRECIPITATION_MODE,i) == vcp_num) {

		    for (j=i;j<19;j++) {

			ORPGVCP_set_wxmode_vcp (PRECIPITATION_MODE,j,
			    ORPGVCP_get_wxmode_vcp (PRECIPITATION_MODE,j+1));

		    }

		    ORPGVCP_set_wxmode_vcp (PRECIPITATION_MODE,19,0);

		}
	    }

	} else if (wx_mode == PRECIPITATION_MODE)  {

/*	The VCP already exists for the other weather mode so we	*
 *	need to remove it.					*/

	    for (i=0;i<20;i++) {

		if (ORPGVCP_get_wxmode_vcp (CLEAR_AIR_MODE,i) == vcp_num) {

		    for (j=i;j<19;j++) {

			ORPGVCP_set_wxmode_vcp (CLEAR_AIR_MODE,j,
			    ORPGVCP_get_wxmode_vcp (CLEAR_AIR_MODE,j+1));

		    }

		    ORPGVCP_set_wxmode_vcp (CLEAR_AIR_MODE,19,0);

		}
	    }
	}

	for (i=0;i<20;i++) {

	    if (ORPGVCP_get_wxmode_vcp (wx_mode,i) == vcp_num) {

		break;

	    } else if (ORPGVCP_get_wxmode_vcp (wx_mode,i) == 0) {

		ORPGVCP_set_wxmode_vcp (wx_mode,i,vcp_num);
		break;

	    }
	} 

/*	Set "where_defined".					*/
        vcp_set = 0;
	if( where_defined & ORPGVCP_RDA_DEFINED_VCP ){

  	    for (i=0;i<20;i++) {

		if (ORPGVCP_get_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP, i ) == vcp_num ){

			vcp_set = 1;
                        break;

		}

	    }

	    if( !vcp_set ){

		for (i=0;i<20;i++) {

		    if( ORPGVCP_get_where_defined_vcp ( ORPGVCP_RDA_DEFINED_VCP, i ) == 0) {

			ORPGVCP_set_where_defined_vcp( ORPGVCP_RDA_DEFINED_VCP, i, vcp_num );
			break;

 		    }

		}

	    }

	}

        vcp_set = 0;
        if( where_defined & ORPGVCP_RPG_DEFINED_VCP ){

            for (i=0;i<20;i++) {

                if (ORPGVCP_get_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP, i ) == vcp_num ){

                        vcp_set = 1;
                        break;

                }

            }

            if( !vcp_set ){

                for (i=0;i<20;i++) {

                    if( ORPGVCP_get_where_defined_vcp ( ORPGVCP_RPG_DEFINED_VCP, i ) == 0) {

                        ORPGVCP_set_where_defined_vcp( ORPGVCP_RPG_DEFINED_VCP, i, vcp_num );
                        break;

                    }

                }

            }

        }

        if( where_defined & ORPGVCP_EXPERIMENTAL_VCP )
           ORPGVCP_set_vcp_is_experimental( vcp_num );

        if( where_defined & ORPGVCP_SITE_SPECIFIC_RPG_VCP )
           ORPGVCP_set_vcp_site_specific( vcp_num, ORPGVCP_RPG_DEFINED_VCP );

        if( where_defined & ORPGVCP_SITE_SPECIFIC_RDA_VCP )
           ORPGVCP_set_vcp_site_specific( vcp_num, ORPGVCP_RDA_DEFINED_VCP );

	return (indx);

/* End of ORPGVCP_add() */
}

/************************************************************************
 *	Description: This function calculates the azimuth rate		*
 *		(num/sec) for an input surveillance and Doppler PRF	*
 *		and pulse count pair.					*
 *									*
 *	Input:  surv_prf_num     - PRF number for surveillance channel	*
 *		surv_pulse_count - Pulse count for surveillance channel	*
 *		dopl_prf_num     - PRF number for Doppler channel	*
 *		dopl_pulse_count - Pulse count for Doppler channel	*
 *	Output: NONE							*
 *	Return: Azimuth rate (num/sec) on success, -1 on error		*
 ************************************************************************/
float ORPGVCP_compute_azimuth_rate ( int surv_prf_num, int surv_pulse_count, 
                                     int dopl_prf_num, int dopl_pulse_count ){

	float	rate;

/*	Validate the input arguments.  Return an error (-1.0) if an	*
 *	invalid argument is detected.					*/

	if ((surv_prf_num     <    0) ||
	    (surv_prf_num     >    3) ||
	    (dopl_prf_num     <    0) ||
	    (dopl_prf_num     >    8) ||
	    (surv_pulse_count <    0) ||
	    (surv_pulse_count > 1023) ||
	    (dopl_pulse_count <    0) ||
	    (dopl_pulse_count > 1023)) {

		return (-1.0);

	}

/*	If the surveillance PRF and pulse count values are positive,	*
 *	then we want to calculate the contribution of the surveillance	*
 *	channel to the azimuth rate calculation.			*/

	rate = 0.0;

	if ((surv_prf_num > 0) && (surv_pulse_count > 0)) {

	    rate = rate + surv_pulse_count/ORPGVCP_get_prf_value(surv_prf_num);

	}

	if ((dopl_prf_num > 0) && (dopl_pulse_count > 0)) {

	    rate = rate + dopl_pulse_count/ORPGVCP_get_prf_value(dopl_prf_num);

	}

/*	So far the rate field indicates how long, in seconds, it takes	*
 *	to sample both channels.  To find an azimuth rate, we need to	*
 *	take the reciprical (i.e., how many of these samples can be	*
 *	done in a second.  If the azimuthal spacing is one degree, then	*
 *	this value should correspond to the number of degree that can	*
 *	be sampled in a second.						*/

	rate = 1.0/rate;

	return (rate);

/* End of ORPGVCP_compute_azimuth_rate() */
}

/************************************************************************

 	Description: This function returns the number of RPG elevations 
		for a given vcp number. The elevation angle values are 
		returned with "elev_angles". If vcp_num < 0, the current 
		VCP is assumed. The size of the "elev_angles" buffer is 
		"buffer_size". If the buffer is too small, only the
		first "buffer_size" elevations are returned.			 
 	Input:  vcp_num - The VCP number
		buffer_size - size of buffer "elev_angles".			
		vol_num - volume scan number (optional)
 	Output: elev_angles - Elevation angles.					
 	Return: Number of elevation for the VCP on success or -1 on 
		failure.
			
 ************************************************************************/
int ORPGVCP_get_all_elevation_angles ( int vcp_num, int buffer_size, 
                                       float *elev_angles, ...) {

    int n_cuts, n_ele, i;
    int volnum_flag = vcp_num & ORPGVCP_VOLNUM;
    unsigned int vol_num = 0xffffffff;

    va_list ap;

/*	If the volnum_flag is set, need to pass this along to 
        other ORPGVCP functions. */
    if( volnum_flag ){

        va_start( ap, elev_angles );
        vol_num = va_arg( ap, unsigned int );
        vol_num %= 2;
        va_end( ap );

    }

    if( vol_num == 0xffffffff )
        n_cuts = ORPGVCP_get_num_elevations( vcp_num );

    else
        n_cuts = ORPGVCP_get_num_elevations( vcp_num, vol_num );

    if (n_cuts < 0)
	return (-1);

    n_ele = 0;
    for (i = 0; i < n_cuts; i++) {
	int new_n;

        if( vol_num == 0xffffffff )
	    new_n = ORPGVCP_get_rpg_elevation_num (vcp_num, i) - 1;
        else
	    new_n = ORPGVCP_get_rpg_elevation_num (vcp_num, i, vol_num) - 1;

	if (new_n < 0)
	    return (-1);
	if (new_n == n_ele - 1)
	    continue;
	if (new_n != n_ele) {
	    LE_send_msg (GL_INFO, "ORPGVCP: Elevation number out of order\n");
	    return (-1);
	}
	if (n_ele < buffer_size) {

            if( vol_num == 0xffffffff )
	        elev_angles[n_ele] = ORPGVCP_get_elevation_angle( vcp_num, i );
            else
	        elev_angles[n_ele] = ORPGVCP_get_elevation_angle( vcp_num, i, vol_num );
	    if (elev_angles[n_ele] == -99.9)
		return (-1);
	}
	n_ele++;
    }

    return (n_ele);
}

/************************************************************************

   Description:
      Take the elevation/azimuth angle in BAMS and convert it into a
      degree value.

   Inputs:
      type - either ORPGVCP_AZIMUTH_ANGLE or ORPGVCP_ELEVATION_ANGLE
      angle_bams - the angle in BAMs

   Returns:
      The angle in degrees.

************************************************************************/
double ORPGVCP_ICD_angle_to_deg( int type, unsigned short angle_bams ){

   double angle_deg;

   angle_deg = (double) (((double) (angle_bams & 0xfff8)) * ORPGVCP_ELVAZM_BAMS2DEG);
   if( type & ORPGVCP_ELEVATION_ANGLE ){

      if( angle_deg > 180.0 )
         angle_deg -= 360.0;
   }

   return( angle_deg );

} /* End of ORPGVCP_ICD_angle_to_deg() */

/************************************************************************
 
   Description:
      Take the elevation/azimuth angle in BAMS and convert it into a 
      degree value such that the value is precise to 0.1 degree.

      If type is OR'd with ORPGVCP_ANGLE_FULL_PRECISION, then the
      value is returned in full floating point precision.

   Inputs:
      type - either ORPGVCP_AZIMUTH_ANGLE or ORPGVCP_ELEVATION_ANGLE
      angle_bams - the angle in BAMs

   Returns:
      The angle in degrees.  If type is not either ORPGVCP_AZIMUTH_ANGLE
      or ORPGVCP_ELEVATION_ANGLE, returns 0xffff.

************************************************************************/
float ORPGVCP_BAMS_to_deg( int type, unsigned short angle_bams ){

   double angle_deg;
   int    angle_deg_int;

   /* Check for correct type. */
   if( !(type & ORPGVCP_ELEVATION_ANGLE) && !(type & ORPGVCP_AZIMUTH_ANGLE) )
      return 0xffff;

   angle_deg = ORPGVCP_ICD_angle_to_deg( type, angle_bams );

   if( type & ORPGVCP_ANGLE_FULL_PRECISION )
      return( (float) angle_deg );

   if( angle_deg > 0 )
      angle_deg_int = (int) ((angle_deg*10.0) + 0.5);
   else
      angle_deg_int = (int) ((angle_deg*10.0) - 0.5);

   return( (float) (angle_deg_int/10.0) );

} /* End of ORPGVCP_BAMS_to_deg() */

/************************************************************************

   Description:
      Take the elevation/azimuth angle in degrees and convert it into a 
      BAM value.

   Inputs:
      type - either ORPGVCP_AZIMUTH_ANGLE or ORPGVCP_ELEVATION_ANGLE
      angle_deg - the angle in degrees

   Returns:
      The angle in BAMS.  On error, returns 0xffff;

************************************************************************/
unsigned short ORPGVCP_deg_to_BAMS( int type, float angle_deg ){

   float scale;
   unsigned short angle_bams_short;
  
   /* Check for correct type. */
   if( !(type & ORPGVCP_ELEVATION_ANGLE) && !(type & ORPGVCP_AZIMUTH_ANGLE) )
      return 0xffff;

   /* For elevation angle, the angle must be within the -1.0 and 45.0 limits. */
   if( type & ORPGVCP_ELEVATION_ANGLE ){

      if( angle_deg < 0.0 ){

        if( (angle_deg - ORPGVCP_HALF_BAM) < -1.0 )
           scale = 0.0;

        else
           scale = -ORPGVCP_HALF_BAM;

      }
      else{

         if( (angle_deg + ORPGVCP_HALF_BAM) > 45.0 )
            scale = 0.0;

         else 
           scale = ORPGVCP_HALF_BAM;

      }

   }
   else
      scale = ORPGVCP_HALF_BAM;

   angle_bams_short = (unsigned short) (((double) (angle_deg + scale)) *
      ORPGVCP_ELVAZM_DEG2BAMS);

  return( (unsigned short) (angle_bams_short & 0xfff8) );

} /* End of ORPGVCP_deg_to_BAMS() */

/************************************************************************

   Description:
      Take the elevation/azimuth rate in BAMS and convert it into a 
      deg/s value.

   Inputs:
      angle_bams - the rate, in BAMS

   Returns:
      The rate, in deg/s.

************************************************************************/
float ORPGVCP_rate_BAMS_to_degs( unsigned short angle_bams ){

   float angle_deg;

   angle_deg = (float) (((double) (angle_bams & 0xfff8)) * ORPGVCP_RATE_BAMS2DEG);

   return( angle_deg );

} /* End of ORPGVCP_rate_BAMS_to_degs() */

/************************************************************************

   Description:
      Take the elevation/azimuth rate in degrees/s and convert it into a 
      BAM value.

   Inputs:
      angle_deg - the rate, in deg/s.

   Returns:
      The rate, in BAMS.

************************************************************************/
unsigned short ORPGVCP_rate_degs_to_BAMS( float angle_deg ){

   unsigned short angle_bams_short;

   angle_bams_short = (unsigned short) (((double)
      (angle_deg + ORPGVCP_RATE_HALF_BAM)) * ORPGVCP_RATE_DEG2BAMS);

   return( (unsigned short) (angle_bams_short & 0xfff8) );

} /* End of ORPGVCP_rate_degs_to_BAMS() */


