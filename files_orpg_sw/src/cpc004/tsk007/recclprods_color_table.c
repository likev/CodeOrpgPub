/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 21:43:44 $
 * $Id: recclprods_color_table.c,v 1.2 2002/11/26 21:43:44 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/*******************************************************************************
Module:        recclprods_color_table.c

Description:   routine to bin values from the REC algorithm (cpc004/tsk007)
               into 11 or 12 color bins for product 132 and 133.
               
               This table was based on the FORTRAN module 
               A3CD7G_SITE_GENERIC_ADAPT_DATA and correlates with Table #1, 
               Reflect 16 level mode A color values.
               
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:
               Initial implementation 1/31/02 - Stern
               2/21/02 - ADS - modified upper loop bounds for both REFL & DOP
                  for color level 10 to be <= so that 100% was included.
               
$Id: recclprods_color_table.c,v 1.2 2002/11/26 21:43:44 nolitam Exp $
*******************************************************************************/


/* local include file */
#include "recclprods_color_table.h"


/*******************************************************************************
Description:      create_rec_color_table is used to load the color binning
                  array used in the REC algorithm. If the binning is for
                  product 132, then 11 bins are provided. Product 133 needs
                  12 bins to account for range folding
                  
Input:            short *clr        array to hold the color bin values
                  int prod_id       132=REC REFL, 133=REC DOP
                  
Output:           short *clr        populated color bin table
                  
Returns:          none
                  
Globals:          none
Notes:            color table array must be allocated in the calling routine.
*******************************************************************************/
void create_rec_color_table(short *clr,int prod_id) {
  /* fill the short array 'clr' (which stands for color) with
  11 or 12 color levels */
  int i;

   if(prod_id==RECCLREF) {
      
      /* color table for 11 level Reflectivity REC */
      for(i=0;i<2;i++)        /* color 0  - no data       */
         clr[i]=0;
      for(i=2;i<12;i++)       /* color 1  - 0 <= % < 10   */
         clr[i]=1;
      for(i=12;i<22;i++)      /* color 2  - 10 <= % < 19  */
         clr[i]=2;
      for(i=22;i<32;i++)      /* color 3  - 20 <= % < 29  */
         clr[i]=3;
      for(i=32;i<42;i++)      /* color 4  - 30 <= % < 39  */
         clr[i]=4;
      for(i=42;i<52;i++)      /* color 5  - 40 <= % < 49  */
         clr[i]=5;
      for(i=52;i<62;i++)      /* color 6  - 50 <= % < 59  */
         clr[i]=6;
      for(i=62;i<72;i++)      /* color 7  - 60 <= % < 69  */
         clr[i]=7;
      for(i=72;i<82;i++)      /* color 8  - 70 <= % < 79  */
         clr[i]=8;
      for(i=82;i<92;i++)      /* color 9  - 80 <= % < 89  */
         clr[i]=9;
      /* the following line was modified on 2/21/02 to    */
      /* include a '<=' for the upper loop bound          */
      for(i=92;i<=102;i++)    /* color 10 - 90 <= % <= 100*/
         clr[i]=10;
      }
    else {
      /* color table for 12 level Doppler REC */
         clr[0]=0;            /* color 0  - no data       */
         clr[1]=11;           /* color 11 - range folding */
      for(i=2;i<12;i++)       /* color 1  - 0 <= % < 10   */
         clr[i]=1;
      for(i=12;i<22;i++)      /* color 2  - 10 <= % < 19  */
         clr[i]=2;
      for(i=22;i<32;i++)      /* color 3  - 20 <= % < 29  */
         clr[i]=3;
      for(i=32;i<42;i++)      /* color 4  - 30 <= % < 39  */
         clr[i]=4;
      for(i=42;i<52;i++)      /* color 5  - 40 <= % < 49  */
         clr[i]=5;
      for(i=52;i<62;i++)      /* color 6  - 50 <= % < 59  */
         clr[i]=6;
      for(i=62;i<72;i++)      /* color 7  - 60 <= % < 69  */
         clr[i]=7;
      for(i=72;i<82;i++)      /* color 8  - 70 <= % < 79  */
         clr[i]=8;
      for(i=82;i<92;i++)      /* color 9  - 80 <= % < 89  */
         clr[i]=9;
      /* the following line was modified on 2/21/02 to    */
      /* include a '<=' for the upper loop bound          */
      for(i=92;i<=102;i++)    /* color 10 - 90 <= % <= 100*/
         clr[i]=10;
      }

}

