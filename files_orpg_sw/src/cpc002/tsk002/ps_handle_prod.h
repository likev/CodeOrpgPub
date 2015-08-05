/**************************************************************************

      Module: ps_handle_prod.h

 Description: DERELICT HEADER FILE

 Assumptions:

 **************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/12/27 16:53:22 $
 * $Id: ps_handle_prod.h,v 1.4 2005/12/27 16:53:22 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef PS_HANDLE_PROD_H
#define PS_HANDLE_PROD_H

/* Structure definition for the Default Product Generation Table used by
   ps_routine. */
typedef struct DEFAULT_TABLE{

   prod_id_t  prod_id; 				  /* Product ID. */
   char       gen_task[ORPG_TASKNAME_SIZ];        /* Generating Task. */
   int        gen_pr;				  /* Product generation period. */
   int        req_num;
   short      params[ NUM_PROD_DEPENDENT_PARAMS]; /* Product dependent parameters. */
   short      elev_index;                         /* Elevation index corresponding 
                                                     to elevation parameter. */
   short      spare;
   struct DEFAULT_TABLE *next;

} Pd_prod_gen_tbl_entry;

#endif /*DO NOT REMOVE!*/
