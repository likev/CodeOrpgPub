/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2004/02/04 21:59:56 $
 * $Id: orpgumc.h,v 1.4 2004/02/04 21:59:56 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */  

/**************************************************************************

      Module: orpgumc.h ( User Message Conversion )

 Description:
        This is the public header file for the orpgumc module. 


 **************************************************************************/
 

#ifndef ORPGUMC_H
#define ORPGUMC_H

#include <prod_user_msg.h>
#include <product.h>


#define UMC_MALLOC_FAILED	-1	
#define UMC_CONVERSION_FAILED	-2
#define UMC_UNKNOWN_TYPE        -3	/* Unknown msg code. */
#define UMC_MSG_LEN_ERROR       -4	/* msg length mismatched. */
#define UNDEF_VALUE             0


/* Change the format from icd->c. */
int UMC_from_ICD( void *icd_msg, int icd_size, int hd_size,void **c_msg );

/* Change the format from c->icd. */
int UMC_to_ICD( void *c_msg, int c_size, int hd_size, void **icd_msg );

int UMC_product_to_icd (void *pmsg, int size);
int UMC_product_from_icd (void *pmsg, int size);
int UMC_product_length (void *prod);
void UMC_message_header_block_swap (void *mhb);
void UMC_msg_hdr_desc_blk_swap (void *mhb);
int UMC_get_product_length (void *phd);
int UMC_product_back_to_icd (void *pmsg, int size);

/* for backward compatibility; identical to UMC_product_from_icd */
int UMC_icd_to_product (void *pmsg, int size);

#endif			/* #ifndef ORPGUMC_H */

















