/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/05/14 16:11:15 $
 * $Id: dp_elev_process.c,v 1.1 2008/05/14 16:11:15 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include "dp_elev_func_prototypes.h"

/******************************************************************************
   Filename: dp_elev_process.c
   
   Description:
   ============
   extract_momentData() copies Dual-pol moment data from the input into 
   the output buffer based on the word size. NOTE: Current dual-pol data 
   are all 8 bit word (gate.b) from HCA, except for the melting layers, 
   DML, which is sent as a 16 bit unsigned short (gate.u_s) to avoid 
   round off.

   Inputs: Generic_moment_t* input - copied from
   
   Outputs: Generic_moment_t* output - copied to
   
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       Liu                Initial implementation 
*****************************************************************************/

void extract_momentData(Generic_moment_t* output, Generic_moment_t* input)
{
   int             i;
   unsigned char*  cpt;
   unsigned short* spt;
   int             verbose = 0;

   cpt = output->gate.b;
   spt = output->gate.u_s;

   if(input->data_word_size == 8) /* 8 bit */
   {
      for (i=0; i < input->no_of_gates; i++)
         cpt[i] = input->gate.b[i];
   }
   else /* 16 bit */ 
   {
      for (i = 0; i < input->no_of_gates; i++ )
         spt[i] = input->gate.u_s[i];
   }

   if(verbose && DP_ELEV_PROD_DEBUG)
   {
     for(i=0; i<input->no_of_gates; i++)
     {
        fprintf(stderr, "input->gate.b[%d] = %d, output->gate.b[%d] = %d\n",
                i, input->gate.b[i], i, output->gate.b[i]);
     }
   }
}
