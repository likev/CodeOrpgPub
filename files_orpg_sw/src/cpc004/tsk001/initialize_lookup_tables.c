/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2003/08/01 19:32:47 $
 * $Id: initialize_lookup_tables.c,v 1.3 2003/08/01 19:32:47 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/**************************************************************************

   initialize_lookup_tables.c

   PURPOSE:

   This routine drives the building of the lookup tables. 

   CALLED FROM:

   apply_mpda1

   INPUTS:

   int rng            - An unambiguous range from the base data (km)
   int rng_nyq_prod   - The product of the nyquist velocity and the unambiguous
                        range from the radar, which will be constant for the
                        radar (km * m/s)

   CALLS:

   build_lookup_table

   OUTPUTS:

   None.

   RETURNS:

   Logical or'd status from build_lookup_table.  15 combinations of prfs are
   possible.  A value of zero (FALSE) means the table was successfully built. 
   A value of 1 (TRUE) means the table was not successfully built.

   NOTES:

   This routine takes an unambiguous range from the base data and calculates
   the possible nyquist values, using the list of possible unambiguous ranges
   from the adaptation data.  Using the possible nyquist velocities, a lookup
   table for unfolding pairs of velocities is prebuilt.
   
   HISTORY:

   R. May, 1/02		- Rewritten for more modularity
   B. Conway, 10/00     - cleanup
   B. Conway, 5/99      - original development

****************************************************************************/

#include <memory.h>
#include <rdacnt.h>
#include "mpda_constants.h"
#include "mpda_structs.h"
#include "mpda_vcp_info.h"

#define INITIAL_PRI_INDEX 99
#define MIDDLE_TABLE 2

int
build_lookup_table(short nyq1, short nyq2, int tab_num);

void
fix_nyq_value(short *nyq);

int 
initialize_lookup_tables(int rng, int rng_nyq_prod)
{
   int tab_num, nyq1, nyq2, i, j, pri_index;
   int mk_status = 0;
   int table_status = 0;
   short nyq_table[MAX_PRFS];
     
/* Determine which crystal set the radar uses */

   pri_index = INITIAL_PRI_INDEX;
   
   for(i=0;i<DELPRI_MAX;i++)
      {
      for(j=0;j<PRFMAX;j++)
         if (rng==range_table[i][j])
         {
         pri_index = i;
         break;
         }
      if (pri_index != INITIAL_PRI_INDEX)
         break;
      }

   if (pri_index == INITIAL_PRI_INDEX)
      pri_index = MIDDLE_TABLE;


/* Generate Nyquists vels from the crystal set and known range and nyquist */

   for(i=2;i<MAX_PRFS+2;i++)
      {
      nyq_table[i-2] =(float)(rng_nyq_prod)/(float)(range_table[pri_index][i]);
      fix_nyq_value(&nyq_table[i-2]);
      }

/* Build unfold table */

   tab_num = 0;
   for(i=0;i<MAX_PRFS-1;i++)
      {
      nyq1=nyq_table[i];
      for(j=i+1;j<MAX_PRFS;j++)
         {
         nyq2=nyq_table[j];
         prf_pairs_table[MAX_PRFS*j+i]=tab_num;
         memset(lookup_table[tab_num].value, MISSING_BYTE,
                sizeof(lookup_table[tab_num].value));
         table_status = build_lookup_table(nyq1, nyq2, tab_num);
         mk_status = mk_status || table_status;
         ++tab_num;
         }
      }
      return mk_status;
}
