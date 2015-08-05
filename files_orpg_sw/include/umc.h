/* 
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:11:35 $
 * $Id: umc.h,v 1.25 2002/12/11 22:11:35 nolitam Exp $
 * $Revision: 1.25 $
 * $State: Exp $
 */  

/**************************************************************************

      Module: umc.h ( User Message Conversion )

 Description:
        This is the header file for the umc.c. 


 **************************************************************************/
 

#ifndef UMC_H
#define UMC_H

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

int UMC_from_ICD_RDA (void *msg, int len);
int UMC_product_to_icd (void *pmsg, int size);
int UMC_product_from_icd (void *pmsg, int size);
int UMC_product_length (void *prod);
void UMC_message_header_block_swap (void *mhb);
int UMC_get_product_length (void *phd);
int UMC_product_back_to_icd (void *pmsg, int size);

/* for backward compatibility; identical to UMC_product_from_icd */
int UMC_icd_to_product (void *pmsg, int size);

#endif			/* #ifndef UMC_H */

















