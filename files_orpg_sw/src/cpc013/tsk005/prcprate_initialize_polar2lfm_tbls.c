/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/09 15:43:56 $
 * $Id: prcprate_initialize_polar2lfm_tbls.c,v 1.1 2005/03/09 15:43:56 ryans Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/****************************************************************************
   Filename: prcprate_initialize_polar2lfm_tbls.c 

   Description
   ===========
     This function initializes polar to lfm conversion tables.

   Change History
   =============
   10/13/92      0000       BRADLEY SUTKER       CCR# NA92-28001
   10/26/92      0000       JOSEPH WHEELER       CCR# NA90-93082
   03/25/93      0001       Toolset              SPR NA93-06801
   01/28/94      0002       Toolset              SPR NA94-01101
   03/03/94      0003       Toolset              SPR NA94-05501
   11/25/04      0004       Cham Pham            CCR NA05-01303
*****************************************************************************/
/* System include files */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Global include file */
#include <a3146.h>

void init_polar_to_lfm_tables( )
{
 int i,j;

/* Initialize conversion table data */
  a314c1.grid_lat = 0;
  a314c1.grid_lon = 0;

  for ( i=0; i<KRADS; i++ ) 
  {

    for ( j=0; j<KBINS; j++ ) 
    {
       a314c1.lfm4grid[i][j] = BEYOND_GRID;
       a314c1.lfm40grid[i][j] = BEYOND_GRID;
    }

    for ( j=0; j<RNG_LFM16; j++ )
    {
       a314c1.lfm16grid[i][j] = BEYOND_GRID;
    }

  }/* End loop KRADS */

  for ( i=0; i<FLAG_SIZE; i++ ) 
  {

    for ( j=0; j<NUM_LFM4; j++ ) 
    {
       a314c1.lfm4flag[i][j]  = BEYOND_RANGE;
    }

    for ( j=0; j<NUM_LFM16; j++ )
    {
       a314c1.lfm16flag[i][j] = BEYOND_RANGE;
    }

    for ( j=0; j<NUM_LFM40; j++ )
    {
       a314c1.lfm40flag[i][j] = BEYOND_RANGE;
    }

  }/* End loop FLAG_SIZE */ 

  a314c1.max82val = 0;
  a314c1.end_lat  = 0;
  a314c1.end_lon  = 0;
}
