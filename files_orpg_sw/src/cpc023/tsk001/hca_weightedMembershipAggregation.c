/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2008/07/21 13:25:16 $ */
/* $Id: hca_weightedMembershipAggregation.c,v 1.2 2008/07/21 13:25:16 cmn Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <hca.h>

/********************************************************************* 

   Description:
      Controls the processing of a radial of input data for the
      Hydrometeor Classification Algorithm (HCA). 

   Inputs:
      outptr - pointer to output buffer.
      inptr - pointer to radial message.

   Returns:
      Nothing

******************************************************************* */
void hca_weightedMembershipAggregation(float weight[NUM_FL_INPUTS], 
                                       float fd_mem[NUM_FL_INPUTS],
                                       float *agg){
    
 /* Local variables */
 int fl_input; /* Loop index for FL Inputs */
 float s = 0.0; /* sum of W*Q */
 float sfd = 0.0; /* sum of W*Q*F(d) */

 s = 0.0;
 sfd = 0.0;
 for(fl_input = 0; fl_input < NUM_FL_INPUTS; fl_input++) {
  s   += (weight[fl_input]*quality[fl_input]);
 }/* END of for(fl_input = 0;...*/


 for(fl_input = 0; fl_input < NUM_FL_INPUTS; fl_input++) {
  sfd += (weight[fl_input]*quality[fl_input]*fd_mem[fl_input])/(s+0.01); 
 }/* END of for(fl_input = 0;...*/

 *agg = sfd;

} /* End of hca_weightedMembershipAggregation() */
