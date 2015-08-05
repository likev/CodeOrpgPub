/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:17 $
 * $Id: hci_prf_product_functions.c,v 1.8 2009/02/27 22:26:17 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/************************************************************************
 *	Module: hci_prf_product_functions.c				*
 *	Description: This module contains a collection of routines	*
 *		     to read and manipulate a PRF obscuration data	*
 *		     product.						*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_prf_product.h>

#define	MAX_PRODUCTS	32000  /* Maximum number of products supported
				* in obscuration LB. */
static	char	*Data_prf  = (char *) NULL; /* buffer for obscuration data */
static	Prfbmap_prod_t	*Prf_data = (Prfbmap_prod_t *) NULL; /* pointer to
				  obscuration data buffer */

static	int	Prf_pointer [8*sizeof(short)] = {-1}; /* Pointer lookup table
				  for various allowable PRFs in obscuration
				  data buffer. */
static  int 	Prf_io_status = 0; /* last read status */

/************************************************************************
 *	Description: This function returns the latest obscuration data	*
 *		     read status.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: last read status					*
 ************************************************************************/

int hci_prf_io_status()
{
	return(Prf_io_status);
}

/************************************************************************
 *	Description: This function loads obscuration data for the	*
 *		     specified date, time, and elevation.		*
 *									*
 *	Input:  date      - julian days since 1/1/1970			*
 *		time      - time (milliseconds past midnight)		*
 *		elevation - elevation angle (deg)			*
 *	Output: NONE							*
 *	Return: read status (negative - error; 0 - product not found;	*
 *			     positive - product found			*
 ************************************************************************/

int
hci_load_prf_product (
int	date,
int	time,
float	elevation
)
{
	int	i;
	int	status;
	int	num_products;
	LB_info	list [MAX_PRODUCTS];
	int	len;
	int	product;
	int	indx;

/*	Use LB_list to get information on number of prf products	*
 *	available in the linear buffer.					*/

	num_products = ORPGDA_list (PRFOVLY, list, MAX_PRODUCTS);
	
	Prf_io_status = num_products;

	if (num_products <= 0) {

	    return 0;

	} 

	product = num_products-1;

	status = 0;

/*	Starting with the newest product in the LB, find the newest	*
 *	product which matches the elevation cut speciied and volume	*
 *	date/time.							*/

	while (product >= 0) {

	    if (Data_prf != (char *) NULL) {

		free (Data_prf);
		Data_prf = (char *) NULL;

	    }

	    Data_prf = (char *) calloc ((int) 2*list [product].size + 1 , 1);

	    if (Data_prf == (char *) NULL) {

		return (-1);

	    }

	    Prf_data = (Prfbmap_prod_t *) (Data_prf+sizeof (Prod_header));

	    len = ORPGDA_read(PRFOVLY,
			Data_prf,
			list[product].size, 
			list[product].id);

	    Prf_io_status = num_products;       
 	    if (len != list [product].size) {

	        if (len <= 0) {

		    HCI_LE_error("LB read error: %d", len);

	        } else {

		    HCI_LE_error("message size expected:%d -  got: %d",
		         list [product].size, len);

	        }

		status = 0;
	        break;

	    }
	    
	    if ((hci_prf_get_date () == date) &&
		(hci_prf_get_time () == time) &&
		(hci_prf_get_elevation () == elevation)) {

/*		This is the elevation cut that we want so lets build	*
 *		a prf lookup table and return.				*/

		indx = 0;

		for (i=0;i<8*sizeof (short);i++) {

		    if ((Prf_data->hdr.prf_nums >> i) & 1) {

			Prf_pointer [i] = i;

		    } else {

			Prf_pointer [i] = -1;

		    }
		}

		status = 1;
		break;

	    }

	    product--;

	}

	if (status == 0) {

	    free (Data_prf);
	    Data_prf = (char *) NULL;

	}

	return status;
}

/************************************************************************
 *	Description: This function returns the volume scan start date	*
 *		     for the loaded prf obscuration product.  If the	*
 *		     data are undefined, then -1 is returned.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: start date (julian days )or -1 if undefined.		*
 ************************************************************************/

int
hci_prf_get_date ()
{
	if (Data_prf == (char *) NULL) {

	    return -1;

	} else {

	    return (int) Prf_data->hdr.date;
	}
}

/************************************************************************
 *	Description: This function returns the volume scan start time	*
 *		     for the loaded prf obscuration product.  If the	*
 *		     data are undefined, then -1 is returned.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: start time (miiliseconds past midnight) or		*
 *		-1 if undefined.					*
 ************************************************************************/

int
hci_prf_get_time ()
{
	if (Data_prf == (char *) NULL) {

	    return -1;

	} else {

	    return (int) Prf_data->hdr.time;
	}
}

/************************************************************************
 *	Description: This function returns the elevation angle for the	*
 *		     loaded prf obscuration product.  If the data are	*
 *		     data are undefined, then -1 is returned.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: elevation angle (degrees) or -1 if undefined.		*
 ************************************************************************/

float
hci_prf_get_elevation ()
{
	if (Data_prf == (char *) NULL) {

	    return -1.0;

	} else {

	    return (float) (Prf_data->hdr.elev_angle/10.0);

	}
}

/************************************************************************
 *	Description: This function returns the data for the specified	*
 *		     prf obscuration product element.  If the data are	*
 *		     data are undefined, then -1 is returned.		*
 *									*
 *	Input:  prf  - PRF number					*
 *		beam - radial index					*
 *		bin  - bin index					*
 *	Output: NONE							*
 *	Return: data (0 or 1) or -1 if error.				*
 ************************************************************************/

int
hci_prf_get_data (
int	prf,
int	beam,
int	bin
)
{
	int	word;
	int	bit;
	int	val;
	int	indx;

	if (Data_prf == (char *) NULL) {

	    return -1;

	} else {

/*	    First check to see if the desired PRF is defined in the	*
 *	    product.							*/

	    indx = hci_prf_get_index (prf);

	    if (indx < 0) {

		return -1;

	    }

/*	    Determine the index of the unsigned char in the prf data	*
 *	    structure containing the bin.				*/

	    word = bin/8;

/*	    Determine the bit within the unsigned char element which	*
 *	    matches the bin.						*/

	    bit  = bin%8;

/*	    Now determine the value (0 or 1) of the element.		*/

	    val = ((Prf_data->bmap [indx].prfbmap [beam].radbmap [word]>>bit) & 1);
	    return (int) val;

	}
}

/************************************************************************
 *	Description: This function returns the bitmap index for the	*
 *		     specified prf in the prf obscuration product.  If	*
 *		     the prf is undefined, -1 is returned.		*
 *									*
 *	Input:  prf  - PRF number					*
 *	Output: NONE							*
 *	Return: index or -1 if error.					*
 ************************************************************************/

int
hci_prf_get_index (
int	prf
)
{
	if ((Data_prf == (char *) NULL) ||
	    (prf  <= 0) ||
	    (prf  >  MAX_ALWBPRFS)) {

	    return -1;

	} else {

	    return (int) Prf_pointer [prf-1];
	}
}
