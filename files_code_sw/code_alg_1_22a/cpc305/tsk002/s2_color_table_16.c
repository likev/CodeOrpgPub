/*
 * RCS info
 * $Author$
 * $Locker$
 * $Date$
 * $Id$
 * $Revision$
 * $State$
 */

/* create 16 level color table */
/************************************************************************
Module:         s2_color_table_16.c          

Description:    This module contains a fills an array of short integers 
                with one of 16 color tables used to encode reflectivity
                radar bin values. This table was translated from the
                FORTRAN module A3CD7G_SITE_GENERIC_ADAPT_DATA and
                correlates with Table #1, Reflect 16 level mode A
                color values.
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                Version 1.0, October 2000
                
Version 1.1   February 2008    T. Ganger
              Created unique filenames with respect to other algorithms.
              
$Id$
************************************************************************/

#include "s2_color_table_16.h"

void create_color_table(short *clr) {
  /* fill the short array 'clr' (which stands for color) with
  16 color levels */
  int i;
  
  for(i=0;i<=75;i++)
    clr[i]=0;
  for(i=76;i<86;i++)
    clr[i]=1;
  for(i=86;i<96;i++)
    clr[i]=2;
  for(i=96;i<106;i++)
    clr[i]=3;
  for(i=106;i<116;i++)
    clr[i]=4;
  for(i=116;i<126;i++)
    clr[i]=5;
  for(i=126;i<136;i++)
    clr[i]=6;
  for(i=136;i<146;i++)
    clr[i]=7;
  for(i=146;i<156;i++)
    clr[i]=8;
  for(i=156;i<166;i++)
    clr[i]=9;
  for(i=166;i<176;i++)
    clr[i]=10;
  for(i=176;i<186;i++)
    clr[i]=11;
  for(i=186;i<196;i++)
    clr[i]=12;
  for(i=196;i<206;i++)
    clr[i]=13;
  for(i=206;i<216;i++)
    clr[i]=14;
  for(i=216;i<255;i++)
    clr[i]=15;
  }
