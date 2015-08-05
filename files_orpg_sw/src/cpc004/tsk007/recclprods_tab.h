/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 22:07:29 $
 * $Id: recclprods_tab.h,v 1.3 2002/11/26 22:07:29 nolitam Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/************************************************************************
Module:        recclprods_tab.h

Description:   include file for the tabular alphanumeric block function of 
               cpc004/tsk007 which creates output for the Radar Echo Classifier.
               This file contains module specific include file listings, 
               function prototypes and constant definitions.
                
CCR#:          NA98-35001
               
Authors:       Andy Stern, Software Engineer, Mitretek Systems
                   astern@mitretek.org
               Tom Ganger, Systems Engineer,  Mitretek Systems
                   tganger@mitretek.org
               Version 1.0, January 2002
               
History:       
               Initial implementation 1/31/02 - Stern
               
$Id: recclprods_tab.h,v 1.3 2002/11/26 22:07:29 nolitam Exp $
************************************************************************/
#ifndef _RECCLPRODS_TAB_H_
#define _RECCLPRODS_TAB_H_

#include <stdio.h>
#include <stdlib.h>

#include <rpgc.h>
#include <rpg_globals.h>  /* includes a309.h for prod_id definitions          */
#include <product.h>      /* for message header block structure               */

/* the following extern definition is needed for recclalg_adapt.h             */
#define EXTERN extern
#include <recclalg_adapt.h>
#include <recclalg_arrays.h>

#define NUM_TAB_PAGES 2        /* number of pages in the TAB block            */
#define LINE_WIDTH 80          /* width of each line of data in the TAB       */
#define REC_SUCCESS 0
#define REC_FAILURE -1
#define FALSE 0
#define TRUE 1

typedef struct {
   short divider;     /* block divider (always -1)                            */
   short block_id;    /* block ID (always 1)                                  */
   int block_length;  /* length of TAB block in bytes                         */
   } TAB_header;

typedef struct {
   short divider;    /* block divider (always -1)                             */
   short num_pages;  /* total number of pages in the TAB product              */
   } alpha_header;

typedef struct {
   short num_char;  /* number of characters per line                          */
   char data[80];   /* container for one line of TAB output                   */
   } alpha_data;

/* local prototypes                                                           */
int generate_TAB(char *,char *,int,int,int);
void calendar_date (short date,int *dd,int *dm,int *dy);
char *msecs_to_string (int time);

#endif


#if 0
       The following is the template used to create the TAB output
--------------------------------------------------------------------------------
                 Radar Echo Classifier Adaptation Data Parameters               
      Date: MM/DD/YYYY    Time: HH:MM    Vol: XX    Elev: XX.X    Page: 1       
                                                                                
AP/Clutter Target Scaling Function Thresholds:                                  
Texture of Reflectivity generating a 0% likelihood                <= XX.X dBZ**2
Texture of Reflectivity generating a 100% likelihood              >= XX.X dBZ**2
Abs. value of Sign of Refl. Change generating a 0% likelihood     >= XX.X       
Abs. value of Sign of Refl. Change generating a 100% likelihood   <= XX.X       
Abs. value of (Refl. Spin Change-50) generating a 0% likelihood    = XX.X       
Abs. value of (Refl. Spin Change-50) generating a 100% likelihood  = XX.X       
Abs. value of Mean Velocity generating a 0% likelihood            >= XX.X m/s   
Abs. value of Mean Velocity generating a 100% likelihood          <= XX.X m/s   
Standard Deviation of Velocity generating a 0% likelihood         >=  X.X m/s   
Standard Deviation of Velocity generating a 100% likelihood       <=  X.X m/s   
Mean Spectrum Width generating a 0% likelihood                    >= XX.X m/s   
Mean Spectrum Width generating a 100% likelihood                  <= XX.X m/s   
--------------------------------------------------------------------------------
          Radar Echo Classifier Adaptation Data Parameters        Page: 2       
AP/Clutter Target Spin Characteristic Thresholds:                               
Spin Change Threshold                                                XX.X dBZ   
Spin Reflectivity Threshold                                          XX.X dBZ   
                                                                                
AP/Clutter Target Category Weighting:                                           
Texture of Reflectivity weight                                        X.XX      
Sign of Reflectivity Change weight                                    X.XX      
Reflectivity Spin Change weight                                       X.XX      
Mean Velocity weight                                                  X.XX      
Standard Deviation of Velocity weight                                 X.XX      
Mean Spectrum Width weight                                            X.XX      
                                                                                
Extents for Radial Processing:                                                  
Azimuthal Extent                                                      X radials 
Reflectivity Range Extent                                             X bins    
Doppler Range Extent                                                  X bins    
--------------------------------------------------------------------------------


#endif

