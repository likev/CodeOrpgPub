/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/10/18 21:48:33 $
 * $Id: orpgdat_api.h,v 1.4 2005/10/18 21:48:33 ryans Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  orpgpat.h						*
 *		This is the global include file for ORPGPAT.		*
 *									*
 ************************************************************************/




#ifndef ORPGDAT_API_H

#define ORPGDAT_API_H

#define	MAX_DAT_TBL_SIZE		ORPGPAT_MAX_PRODUCT_CODE

/* Data compression type macro definitions. */
#define COMPRESSION_NONE        	0
#define COMPRESSION_BZIP2       	1

#define ORPGDAT_ERROR	       		-850 
#define ORPGDAT_CALLOC_FAILED		-851
#define ORPGDAT_ENTRY_NOT_FOUND		-852

/*	Functions dealing with product attributes table		*/

void	ORPGDAT_error (void (*user_exception_callback)());
int	ORPGDAT_read_tbl  ();

char*	ORPGDAT_get_tbl_ptr(int indx);
Mrpg_data_t* ORPGDAT_get_entry( int data_id, int *size );
int	ORPGDAT_get_compression_type( int data_id );
Mrpg_wp_item* ORPGDAT_get_write_permission( int data_id, int *size );


#endif
