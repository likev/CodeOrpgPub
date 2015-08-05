/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 21:43:47 $
 * $Id: recclprods_helpers.c,v 1.3 2002/11/26 21:43:47 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* recclprods_helpers.c */
/*******************************************************************************
Module:        recclprods_helpers.c

Description:   helper modules common to both the reflectivity and doppler
               portions of cpc004/tsk007 - REC (Radar Echo Classifier) task.
               
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_helpers.c,v 1.3 2002/11/26 21:43:47 nolitam Exp $
*******************************************************************************/


/* local include file */
#include "recclprods_helpers.h"


/*******************************************************************************
Description:      radials_to_process is a routine that looks through the radial
                  header data structure for the -1 flag which indicates "no
                  data". when this flag is reached the total number of
                  populated radials can be found. used to check for array
                  bounds.
                  
Input:            char* inbuf       pointer to the input buffer containing the
                                    intermediate data buffer
                  
Output:           none
                  
Returns:          returns the total number of radials to process
                  
Globals:          none
Notes:            none
*******************************************************************************/
int radials_to_process(char *inbuf) {
   /* determine the total number of radials to process. the key
   is to look at the radial header portion of the input buffer and
   read in the azimuth portion until the value becomes 0xFFFF. The
   total up to this point represents the actual number of populated
   radials. return this value */
   int offset=sizeof(Rec_prod_header_t);
   
   Rec_rad_header_t *rptr=(Rec_rad_header_t*)(inbuf+offset);
   int radial_count=0;
   int i;
   int DEBUG = FALSE;
   
   if(DEBUG)
      fprintf(stderr,"inside radials_to_process\n");
   
   for(i=0;i<MAX_RADIALS;i++) {
         
      if(rptr->azimuth>=0) 
         radial_count++;
        else
         break;
      /*   
      fprintf(stderr,"radial %d azimuth=%d or %02X hex\n",i+1,rptr->azimuth,
         rptr->azimuth);         
      */         
      rptr++;
      }

   if(DEBUG)
      fprintf(stderr,"returned number of radials=%d\n",radial_count);   
   return(radial_count);   
   }



/*******************************************************************************
Description:      get_max_dop takes the current pointer to the radial buffer
                  and returns the maximum value of the next 4 bin values.
                  Used only in the Doppler product generation (product 133)
                  
Input:            short *radial     pointer to a buffer holding bin data
                  
Output:           none
                  
Returns:          returns the maximum value of 4 data bins
                  
Globals:          none
Notes:            none
*******************************************************************************/
short get_max_dop(short *radial) {
   /* read in 4 short integers and return the maximum value */
   int i;
   short max=0;
   
   for(i=0;i<4;i++) {
      if(radial[i]>max) max=radial[i];
      }
   
   return(max);   
   }


