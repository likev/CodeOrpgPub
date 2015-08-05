/*
 * RCS info
 * $Author: steves $
 * $Date: 2003/07/09 16:21:38 $
 * $Locker:  $
 * $Id: superob_update_elev_table.c,v 1.4 2003/07/09 16:21:38 steves Exp $
 * $revision$
 * $state$
 * $Logs$
 */

/************************************************************************
 *      Module:         superob_update_elev_table.c                             *
 *                                                                      *
 *      Description: this function is used to update an elevation table *
 *		     it is called by superob.c				*
 *                                                                      *
 *      Input:       max_num_elev: maximum number of elevation angles   *
 *		     elevation:	   elevation angle                      *
 * 		     elev_table:   an array of elevation table		*
 *      Output:      elev_table:   updated elevation table              *
 *                   num_elev:     number of elements in this elev table*
 *                   itilt:        index of this elevation in the table *
 *      returns:     return a negative value if an error is encountered *
 *      Globals:     none                                               *
 *      Notes:       %			                                *
 ************************************************************************/

/***	System include file	         				*/

# include <stdio.h>
# include <math.h>
# include <rpgc.h>

# define TRUE		1   /* logic value fo true	  	        */
# define FALSE		0   /* logic value for false          		*/
# define THRESHOLD_ELEV	0.35/* threshold  of difference between two     *
                             * sequential elevations 			*/

int 
superob_update_elev_table(float elev_table[], int max_num_elev, 
                          int   *num_elev,    int *itilt,
                          float elevation,    int elev_idx)
{
  /* declare local variables for this algorithm                        */
  
  int find_it;               /* find_it==TRUE: this elev close enogh   *
                              * with one element in the table          */
  int i;                     /* Looop index                            */

  /* make sure 'num_elev' is not larger than 'max_num_elev'            */
  if(*num_elev > max_num_elev)
  {
   RPGC_log_msg(GL_ERROR,"ERRO: More than %d elevations for this Radar\n", max_num_elev);
   return -1;
  }

  find_it = FALSE;
  if(*num_elev == 0)         /* the first element put in the table      */
    {
    *itilt = *num_elev;      /* set the index of the eleve to 0         */
    elev_table[*num_elev] = elevation;
    (*num_elev)++;           /* update the num of elements in the table */
    find_it = TRUE;          /* set find_it to TRUE			*/
    }
    else                     /* it is not the first element in the table*/
    {
    for(i=0; i<*num_elev; i++)/* search the close-enough element in the table*/
      {
       if(fabs(elev_table[i]-elevation) < THRESHOLD_ELEV)
         {                   /* find the close-enough element in the table*/
          *itilt = i;        /* set the index of this elev with index of  *
                              * matched elev				  */
          find_it = TRUE;
          break;
         }
      }
    }
   if(!find_it){              /* it is neither the first elev nor the elev*
                              * in the table				 */
     if(*num_elev <max_num_elev)
       {
        /* add this elev to the end of the table   */
        *itilt = *num_elev;  
        elev_table[*num_elev] = elevation;
        (*num_elev)++;
       }
     else                    /* num of elements in the table larger      *
                              * than the max num of the table            */
       {
       return -1; 
       }
   }
return 1;
}
