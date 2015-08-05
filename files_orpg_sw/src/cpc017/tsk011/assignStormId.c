/*******************************************************************************
    Filename:   assignStormId.c
    Author:     Brian Klein


    Description
    ===========
    This function was modeled after A318N8__MESO_CORSTR and will match storm
    IDs from the TRFRCATR intermediate product linear buffer with the features
    in the provided cplt_t structure.
    
*******************************************************************************/

/* 
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2007/05/25 16:46:08 $
 * $Id: assignStormId.c,v 1.2 2007/05/25 16:46:08 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*** Global Include Files ***/

#include <math.h>
#include <rpgc.h> 


/*** Local Include Files ***/

#include "mdattnn_params.h"
#include "trfrcatr.h"

extern tracking_t scit_tracks;

/*** Static Variable Declarations ***/

const static int    NUM_ALPHA = 26;   /* number of letters in the alphabe */
const static float  MESO_MAX_DIST_ASSOC = 20.0;  /* Max SCIT cell association distance (km)*/
const static char   alpha[]   = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";/* ID, char0 */
const static char   numbers[] = "0123456789";                /* ID, char1 */



void assignStormId
(
   cplt_t* ptrCplt  /* (IN) Feature needing storm ID  */
)
{
    /*** Variable Declarations and Definitions ***/

    int    stm, storm_num;
    int    n_char;         /* int of the numerical (right) character */
    int    c_char;         /* int of the alpha (left) character      */
    float  min_dist;       /* min distance found between feature and storm */
    float  dist_x, dist_y; /* x,y distances between feature and storm */
    float  dist;           /* distance between feature and storm      */
    
   
    /* Initialize storm number                                   */

    storm_num = UNDEFINED;
     
    /* Initialize the minimum distance to a large value.         */

    min_dist = 999.0;

    for (stm = 0; stm < scit_tracks.hdr.bnt; stm++)
    {
       /* Calculate the distance between feature and all storms. */
       dist_x = scit_tracks.bsm[stm].x0 - ptrCplt->llx;
       dist_y = scit_tracks.bsm[stm].y0 - ptrCplt->lly;
          
       dist   = sqrt(dist_x * dist_x + dist_y * dist_y);
          
       /* Determine storm closest to meso.                    */

       if ((dist < min_dist) && (dist < MESO_MAX_DIST_ASSOC))
       {
          min_dist  = dist;
          storm_num = scit_tracks.bsi[stm].num_id;
       }
    } /* end for all storms */
      
    if (storm_num != UNDEFINED)
    {
       /* Set the ASCII storm ID based on the saved numerical ID.         */
       /* This strange code emulates the CHARIDTABLE in A309.INC.  ASCII  */
       /* storm IDs start at A0 for numerical ID #1 then B0 for numerical */
       /* ID #2 and so on until Z0.  The next storm is A1 and so on...    */
       /* (Using integer arithmetic for truncation)                       */
        
       n_char = (storm_num - 1 ) / NUM_ALPHA;
       ptrCplt->storm_id[1] = numbers[n_char];
        
       c_char = (storm_num - (n_char * NUM_ALPHA)) - 1;
       ptrCplt->storm_id[0] = alpha[c_char];
        
       ptrCplt->storm_id[2] = '\0'; /* Null terminate for string use */
    }
    else
    {
       /* Set the ID to question marks                               */
       ptrCplt->storm_id[0] = '?';
       ptrCplt->storm_id[1] = '?';
       ptrCplt->storm_id[2] = '\0'; /* Null terminate for string use */            
    } /* end if storm association found */
          
    return;
} /* end assignStormId() */
