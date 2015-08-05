/*
 * RCS info
 * $Author: dzittel $
 * $Locker:  $
 * $Date: 2005/02/17 16:03:29 $
 * $Id: build_saa_color_tables.c,v 1.2 2005/02/17 16:03:29 dzittel Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        build_saa_color_tables.c

Description:  	 This code creates 16-level lookup tables for low scale snow water
		 equivalent and snow depth products and for high scale snow water
		 equivalent and snow depth products.  The products are the 1 hour 
		 running total snow water equivalent (OSW) and depth (OSD), storm
		 total snow water equivalent (SSW) and snow depth (SSD), and the
		 user selectable snow water equivalent (USW) and snow depth (USD).
		 The USW and USD products use the low scales unless the low scale
		 maximum value is exceeded.  At that point they switch to the high
		 scale.
               
CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, Septembert 2003
               
History:
               Initial implementation 09/24/2003 - Zittel
               11/04/2004	SW CCR NA04-30810	Build8 Changes, add new color tables
               
*******************************************************************************/
#include "build_saa_color_tables.h"
#include <stdio.h>

short su_hi_dcat[16] = {1,10,20,40,60,80,100,120,160,200,240,
                               300,400,500,600,MAX_DPTH};  /* 10th of inches, Build8 */
short hi_dcat[16] = {1,5,10,20,30,40,50,60,80,100,120,
                               150,200,250,300,MAX_DPTH};  /* 10th of inches  */
short lo_dcat[16] = {1,5,10,15,25,50,75,100,150,200,250,
                              300,350,400,500,MAX_DPTH};   /* 100th of inches */ 
short su_hi_wcat[16] = {1,10,20,30,40,50,60,80,100,150,200,
                               250,300,400,500,MAX_WEQV};  /* 100th of inches, Build8 */
short hi_wcat[16] = {1,5,10,15,20,25,30,40,50,75,100,
                               125,150,200,250,MAX_WEQV};  /* 100th of inches */
short lo_wcat[16] = {1,10,20,30,50,70,90,110,130,160,200,
                               250,300,350,400,MAX_WEQV};  /* 1000th of inches */


void build_saa_color_tables(void)
{
   /*  Build the super high depth color table           */
      int i;
      /* Next 30 lines added for Build8                 */
      shdpth_clr[0]=0;                          	/* color 0   */
      for(i=su_hi_dcat[0];i<su_hi_dcat[1];i++)        	/* color 1   */
         shdpth_clr[i]=1;
      for(i=su_hi_dcat[1];i<su_hi_dcat[2];i++)        	/* color 2   */
         shdpth_clr[i]=2;
      for(i=su_hi_dcat[2];i<su_hi_dcat[3];i++)        	/* color 3   */
         shdpth_clr[i]=3;
      for(i=su_hi_dcat[3];i<su_hi_dcat[4];i++)        	/* color 4   */
         shdpth_clr[i]=4;
      for(i=su_hi_dcat[4];i<su_hi_dcat[5];i++)        	/* color 5   */
         shdpth_clr[i]=5;
      for(i=su_hi_dcat[5];i<su_hi_dcat[6];i++)        	/* color 6   */
         shdpth_clr[i]=6;  
      for(i=su_hi_dcat[6];i<su_hi_dcat[7];i++)        	/* color 7   */
         shdpth_clr[i]=7;
      for(i=su_hi_dcat[7];i<su_hi_dcat[8];i++)        	/* color 8   */
         shdpth_clr[i]=8;
      for(i=su_hi_dcat[8];i<su_hi_dcat[9];i++)        	/* color 9   */
         shdpth_clr[i]=9;
      for(i=su_hi_dcat[9];i<su_hi_dcat[10];i++)       	/* color 10  */
         shdpth_clr[i]=10;
      for(i=su_hi_dcat[10];i<su_hi_dcat[11];i++)      	/* color 11  */
         shdpth_clr[i]=11;
      for(i=su_hi_dcat[11];i<su_hi_dcat[12];i++)      	/* color 12  */
         shdpth_clr[i]=12;
      for(i=su_hi_dcat[12];i<su_hi_dcat[13];i++)      	/* color 13  */
         shdpth_clr[i]=13;
      for(i=su_hi_dcat[13];i<su_hi_dcat[14];i++)      	/* color 14  */
         shdpth_clr[i]=14;
      for(i=su_hi_dcat[14];i<su_hi_dcat[15];i++)      	/* color 15  */
         shdpth_clr[i]=15;

   /* Build the high depth color lookup table  */

      hdpth_clr[0]=0;                           /* color 0   */
      for(i=hi_dcat[0];i<hi_dcat[1];i++)        /* color 1   */
         hdpth_clr[i]=1;
      for(i=hi_dcat[1];i<hi_dcat[2];i++)        /* color 2   */
         hdpth_clr[i]=2;
      for(i=hi_dcat[2];i<hi_dcat[3];i++)        /* color 3   */
         hdpth_clr[i]=3;
      for(i=hi_dcat[3];i<hi_dcat[4];i++)        /* color 4   */
         hdpth_clr[i]=4;
      for(i=hi_dcat[4];i<hi_dcat[5];i++)        /* color 5   */
         hdpth_clr[i]=5;
      for(i=hi_dcat[5];i<hi_dcat[6];i++)        /* color 6   */
         hdpth_clr[i]=6;  
      for(i=hi_dcat[6];i<hi_dcat[7];i++)        /* color 7   */
         hdpth_clr[i]=7;
      for(i=hi_dcat[7];i<hi_dcat[8];i++)        /* color 8   */
         hdpth_clr[i]=8;
      for(i=hi_dcat[8];i<hi_dcat[9];i++)        /* color 9   */
         hdpth_clr[i]=9;
      for(i=hi_dcat[9];i<hi_dcat[10];i++)       /* color 10  */
         hdpth_clr[i]=10;
      for(i=hi_dcat[10];i<hi_dcat[11];i++)      /* color 11  */
         hdpth_clr[i]=11;
      for(i=hi_dcat[11];i<hi_dcat[12];i++)      /* color 12  */
         hdpth_clr[i]=12;
      for(i=hi_dcat[12];i<hi_dcat[13];i++)      /* color 13  */
         hdpth_clr[i]=13;
      for(i=hi_dcat[13];i<hi_dcat[14];i++)      /* color 14  */
         hdpth_clr[i]=14;
      for(i=hi_dcat[14];i<hi_dcat[15];i++)      /* color 15  */
         hdpth_clr[i]=15;
	/* Two lines removed for Build8  */

   /*  Build the low depth color table  */

      ldpth_clr[0]=0;                           /* color 0   */
      for(i=lo_dcat[0];i<lo_dcat[1];i++)        /* color 1   */
         ldpth_clr[i]=1;
      for(i=lo_dcat[1];i<lo_dcat[2];i++)        /* color 2   */
         ldpth_clr[i]=2;
      for(i=lo_dcat[2];i<lo_dcat[3];i++)        /* color 3   */
         ldpth_clr[i]=3;
      for(i=lo_dcat[3];i<lo_dcat[4];i++)        /* color 4   */
         ldpth_clr[i]=4;
      for(i=lo_dcat[4];i<lo_dcat[5];i++)        /* color 5   */
         ldpth_clr[i]=5;
      for(i=lo_dcat[5];i<lo_dcat[6];i++)        /* color 6   */
         ldpth_clr[i]=6;  
      for(i=lo_dcat[6];i<lo_dcat[7];i++)        /* color 7   */
         ldpth_clr[i]=7;
      for(i=lo_dcat[7];i<lo_dcat[8];i++)        /* color 8   */
         ldpth_clr[i]=8;
      for(i=lo_dcat[8];i<lo_dcat[9];i++)        /* color 9   */
         ldpth_clr[i]=9;
      for(i=lo_dcat[9];i<lo_dcat[10];i++)       /* color 10  */
         ldpth_clr[i]=10;
      for(i=lo_dcat[10];i<lo_dcat[11];i++)      /* color 11  */
         ldpth_clr[i]=11;
      for(i=lo_dcat[11];i<lo_dcat[12];i++)      /* color 12  */
         ldpth_clr[i]=12;
      for(i=lo_dcat[12];i<lo_dcat[13];i++)      /* color 13  */
         ldpth_clr[i]=13;
      for(i=lo_dcat[13];i<lo_dcat[14];i++)      /* color 14  */
         ldpth_clr[i]=14;
      for(i=lo_dcat[14];i<lo_dcat[15];i++)      /* color 15  */
         ldpth_clr[i]=15;
	/* Two lines removed for Build8  */

   /* Build the super high water equivalent color lookup table  */

      /* Next 30 lines added for Build8                         */
      shweqv_clr[0]=0;                                /* color 0   */
      for(i=su_hi_wcat[0];i<su_hi_wcat[1];i++)        /* color 1   */
         shweqv_clr[i]=1;
      for(i=su_hi_wcat[1];i<su_hi_wcat[2];i++)        /* color 2   */
         shweqv_clr[i]=2;
      for(i=su_hi_wcat[2];i<su_hi_wcat[3];i++)        /* color 3   */
         shweqv_clr[i]=3;
      for(i=su_hi_wcat[3];i<su_hi_wcat[4];i++)        /* color 4   */
         shweqv_clr[i]=4;
      for(i=su_hi_wcat[4];i<su_hi_wcat[5];i++)        /* color 5   */
         shweqv_clr[i]=5;
      for(i=su_hi_wcat[5];i<su_hi_wcat[6];i++)        /* color 6   */
         shweqv_clr[i]=6;  
      for(i=su_hi_wcat[6];i<su_hi_wcat[7];i++)        /* color 7   */
         shweqv_clr[i]=7;
      for(i=su_hi_wcat[7];i<su_hi_wcat[8];i++)        /* color 8   */
         shweqv_clr[i]=8;
      for(i=su_hi_wcat[8];i<su_hi_wcat[9];i++)        /* color 9   */
         shweqv_clr[i]=9;
      for(i=su_hi_wcat[9];i<su_hi_wcat[10];i++)       /* color 10  */
         shweqv_clr[i]=10;
      for(i=su_hi_wcat[10];i<su_hi_wcat[11];i++)      /* color 11  */
         shweqv_clr[i]=11;
      for(i=su_hi_wcat[11];i<su_hi_wcat[12];i++)      /* color 12  */
         shweqv_clr[i]=12;
      for(i=su_hi_wcat[12];i<su_hi_wcat[13];i++)      /* color 13  */
         shweqv_clr[i]=13;
      for(i=su_hi_wcat[13];i<su_hi_wcat[14];i++)      /* color 14  */
         shweqv_clr[i]=14;
      for(i=su_hi_wcat[14];i<su_hi_wcat[15];i++)      /* color 15  */
         shweqv_clr[i]=15;

   /* Build the high water equivalent color lookup table  */

      hweqv_clr[0]=0;                           /* color 0   */
      for(i=hi_wcat[0];i<hi_wcat[1];i++)        /* color 1   */
         hweqv_clr[i]=1;
      for(i=hi_wcat[1];i<hi_wcat[2];i++)        /* color 2   */
         hweqv_clr[i]=2;
      for(i=hi_wcat[2];i<hi_wcat[3];i++)        /* color 3   */
         hweqv_clr[i]=3;
      for(i=hi_wcat[3];i<hi_wcat[4];i++)        /* color 4   */
         hweqv_clr[i]=4;
      for(i=hi_wcat[4];i<hi_wcat[5];i++)        /* color 5   */
         hweqv_clr[i]=5;
      for(i=hi_wcat[5];i<hi_wcat[6];i++)        /* color 6   */
         hweqv_clr[i]=6;  
      for(i=hi_wcat[6];i<hi_wcat[7];i++)        /* color 7   */
         hweqv_clr[i]=7;
      for(i=hi_wcat[7];i<hi_wcat[8];i++)        /* color 8   */
         hweqv_clr[i]=8;
      for(i=hi_wcat[8];i<hi_wcat[9];i++)        /* color 9   */
         hweqv_clr[i]=9;
      for(i=hi_wcat[9];i<hi_wcat[10];i++)       /* color 10  */
         hweqv_clr[i]=10;
      for(i=hi_wcat[10];i<hi_wcat[11];i++)      /* color 11  */
         hweqv_clr[i]=11;
      for(i=hi_wcat[11];i<hi_wcat[12];i++)      /* color 12  */
         hweqv_clr[i]=12;
      for(i=hi_wcat[12];i<hi_wcat[13];i++)      /* color 13  */
         hweqv_clr[i]=13;
      for(i=hi_wcat[13];i<hi_wcat[14];i++)      /* color 14  */
         hweqv_clr[i]=14;
      for(i=hi_wcat[14];i<hi_wcat[15];i++)      /* color 15  */
         hweqv_clr[i]=15;
	/* Two lines removed for Build8  */

   /* Build the low water equivalent color lookup table  */

      lweqv_clr[0]=0;                           /* color 0   */
      for(i=lo_wcat[0];i<lo_wcat[1];i++)        /* color 1   */
         lweqv_clr[i]=1;
      for(i=lo_wcat[1];i<lo_wcat[2];i++)        /* color 2   */
         lweqv_clr[i]=2;
      for(i=lo_wcat[2];i<lo_wcat[3];i++)        /* color 3   */
         lweqv_clr[i]=3;
      for(i=lo_wcat[3];i<lo_wcat[4];i++)        /* color 4   */
         lweqv_clr[i]=4;
      for(i=lo_wcat[4];i<lo_wcat[5];i++)        /* color 5   */
         lweqv_clr[i]=5;
      for(i=lo_wcat[5];i<lo_wcat[6];i++)        /* color 6   */
         lweqv_clr[i]=6;  
      for(i=lo_wcat[6];i<lo_wcat[7];i++)        /* color 7   */
         lweqv_clr[i]=7;
      for(i=lo_wcat[7];i<lo_wcat[8];i++)        /* color 8   */
         lweqv_clr[i]=8;
      for(i=lo_wcat[8];i<lo_wcat[9];i++)        /* color 9   */
         lweqv_clr[i]=9;
      for(i=lo_wcat[9];i<lo_wcat[10];i++)       /* color 10  */
         lweqv_clr[i]=10;
      for(i=lo_wcat[10];i<lo_wcat[11];i++)      /* color 11  */
         lweqv_clr[i]=11;
      for(i=lo_wcat[11];i<lo_wcat[12];i++)      /* color 12  */
         lweqv_clr[i]=12;
      for(i=lo_wcat[12];i<lo_wcat[13];i++)      /* color 13  */
         lweqv_clr[i]=13;
      for(i=lo_wcat[13];i<lo_wcat[14];i++)      /* color 14  */
         lweqv_clr[i]=14;
      for(i=lo_wcat[14];i<lo_wcat[15];i++)      /* color 15  */
         lweqv_clr[i]=15;
	/* Two lines removed for Build8  */
}     
