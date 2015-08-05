/*	hci_rda_adaptation_data.h - This header file defines		*
 *	functions used to access rda adaptation data.			*/

/*
 * RCCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/11/29 23:50:10 $
 * $Id: hci_rda_adaptation_data.h,v 1.12 2006/11/29 23:50:10 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

#ifndef HCI_RDA_ADAPTATION_DATA_DEF
#define	HCI_RDA_ADAPTATION_DATA_DEF

/*	Include files needed.						*/

#include <time.h>
#include <infr.h>
#include <orpgdat.h>
#include <rdacnt.h>
#include <vcp.h>

#define	HCI_VCP_SHORT_PULSE		2
#define	HCI_VCP_LONG_PULSE		4

/*  	Report the status of the last RDA adaptation data function */
int 	hci_rda_adaptation_io_status();

/*  	Report the status of the last RDA VCP data function */
int 	hci_rda_vcp_io_status();

/*	The following functions handle RDA adaptation data I/O.		*/
int	hci_read_rda_adaptation_data ();
int	hci_write_rda_adaptation_data ();

/*	The following function handles RDA VCP data read.		*/
int	hci_read_vcp_adaptation_data ();

/*	The following functions deal with the wx mode table.		*/
short	*hci_rda_adapt_wxmode_table_ptr ();
int	hci_rda_adapt_wxmode (int mode, int pos);

/*	The following functions deal with the VCP where defined table.	*/
short	*hci_rda_adapt_where_defined_table_ptr ();
int	hci_rda_adapt_where_defined (int comp, int vcp_num);

/*	The following function definitions deal with the VCP table.	*/
short	*hci_rda_adapt_vcp_table_ptr (int pos);
int	hci_rda_adapt_vcp_table_vcp_num (int pos);
int	hci_rda_adapt_vcp_table_pulse_width (int pos);
int	hci_rda_adapt_vcp_table_index (int vcp);

/*	The following function deals with the RDA VCP table.	*/
short	*hci_rda_vcp_ptr ();

/*	The following functions deal with RPG elevation indicies.	*/
short	*hci_rda_adapt_elev_indicies_ptr ();

/*	The following functions deal with the Clutter Censor Zones	*
 *	data.								*/
/*	NOTE:  This data structure has been extracted and put into	*
 *	a separate LB.  All future references to these data should	*
 *	point to that LB instead of here.				*/
short	*hci_rda_adapt_clutter_censor_zones_ptr ();

/*	The following functions deal with the allowable PRF table.	*
 *	There is a 1 to 1 mapping to each VCP in the RDA adaptation	*
 *	data.								*/
short	*hci_rda_adapt_allowable_prf_ptr        (int pos);
int	hci_rda_adapt_allowable_prf_vcp_num     (int pos);
int	hci_rda_adapt_allowable_prf_prfs        (int pos);
int	hci_rda_adapt_allowable_prf_num         (int pos, int indx);
int	hci_rda_adapt_allowable_prf_pulse_count (int pos, int elev_num,
						 int prf);

/*	The following functions deal with the PRF table.		*/

float	*hci_rda_adapt_prf_value_ptr ();

/*	The following functions deal with the VCP times table.		*/

short	*hci_rda_adapt_vcp_times_ptr ();

/*	The following functions deal with the Unambiguous Range table.	*/

int	*hci_rda_adapt_unambiguous_range_ptr ();

int	hci_rda_adapt_delta_pri ();

/*	The following function returns the elevation angle associated	*
 *	with a selected VCP and elevation cut.				*/

float	hci_rda_adapt_vcp_elevation_angle (int vcp_num, int cut);

#endif
