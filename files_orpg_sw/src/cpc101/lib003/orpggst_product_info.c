/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 21:12:52 $
 * $Id: orpggst_product_info.c,v 1.4 2002/12/11 21:12:52 nolitam Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/********************************************************************************
   
   Module:  orpggst_product_info.c
   
   Description:  This is the module for getting product distribution information
   status for other routines. 
   

   Assumptions:
      
   ******************************************************************************/

/*
* System Include Files/Local Include Files
*/
#include <orpggst.h>
#include <prod_distri_info.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/


/*
* Static Globals
*/


/*
* Static Function Prototypes
*/


/**************************************************************************
   Description:  This routine reads the product distribution information.
      
   Input: 
      
   Output:
      
   Returns: buf - Pointer to a buffer containing prod_distri_info.
   
   Notes: This routine returns more information than is used by the calling
   routine but was written to be used by other routines if needed.

   **************************************************************************/
   
Pd_distri_info *ORPGGST_get_prod_distribution_info()
{
    char *buf;
       
    Pd_distri_info *p_tbl;
    
    int ret;
    
       	/* read the message */
	ret = ORPGDA_read (ORPGDAT_PROD_INFO, &buf, LB_ALLOC_BUF, 
						PD_LINE_INFO_MSG_ID);	 
	
	if (ret < sizeof (Pd_distri_info)) {
		LE_send_msg (GL_LB(ret), "Error in PD_LINE_INFO_MSG_ID message");
		return (NULL);
		}
	
	/* Return the product distribution table*/
	p_tbl = (Pd_distri_info*)buf;
	return (p_tbl);	
} /* End get prod distribution info */


/**************************************************************************
   Description:  This routine saves the product distribution information.
      
   Input: Pointer to product info 
      
   Output:
      
   Returns: 
   
   Notes: 

   **************************************************************************/
   
int ORPGGST_save_prod_distribution_info(Pd_distri_info *p_tbl, int len)
{
   
    int ret;
   
    
      ret = ORPGDA_write (ORPGDAT_PROD_INFO, (char*)p_tbl, len, 
						PD_LINE_INFO_MSG_ID);
	if (ret <= 0) {
	    LE_send_msg (GL_ORPGDA (ret), 
		"ORPGDA_write PD_LINE_INFO_MSG_ID failed (ret %d)\n", ret);
	    return (-1);
	} 
    
	return (1);	
	
} /* End save prod distribution info */


