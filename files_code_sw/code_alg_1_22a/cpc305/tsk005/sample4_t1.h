/*
 * RCS info
 * $Author$
 * $Locker$
 * $Date$
 * $Id$
 * $Revision$
 * $State$
 */

/************************************************************************
Module:         sample4_t1.h

Authors:        Steve Smith, Software Engineer, NWS ROC
                    steve.smith@noaa.nssl.gov
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                Version 1.0, January 2002

Version 1.1   April 2005   T. Ganger
              Modified to use new product IDs 

Version 1.2   March 2006   T. Ganger
              Eliminated constant output buffer size.   

Version 1.3   March 2007   T. Ganger
              Removed buffer name defines since by_name functions are used.
              
Version 1.4   February 2008    T. Ganger  (Sample Algorithm ver 1.18)
              Eliminated definition of output buffer ids.
                
$Id$
************************************************************************/

#ifndef _SAMPLE4_T1_H_
#define _SAMPLE4_T1_H_

#include <unistd.h>

/* ORPG   includes ----------------------------------------------------*/
#include <rpg_globals.h>

/*** the following are included in rpg_globals.h */
/***                                             */
/***  #include <stdio.h>                         */
/***  #include <stdlib.h>                        */
/***  #include <time.h>                          */
/***  #include <ctype.h>                         */
/***  #include <string.h>                        */
/***                                             */
/***  #include <a309.h>                          */
/***  #include <basedata.h>                      */
/***  #include <basedata_elev.h>                 */
/***  #include <rpg_port.h>                      */
/***  #include <prod_gen_msg.h>                  */
/***  #include <prod_request.h>                  */
/***  #include <prod_gen_msg.h>                  */
/***  #include <rpg_vcp.h>                       */
/***  #include <orpg.h>                          */
/***  #include <mrpg.h>                          */
/*************************************************/


#include <basedata.h>

/* required by rpgc after API enhancement patch */
#include <gen_stat_msg.h>

#include <rpgc.h>
#include "rpgcs.h"





/* local defines -----------------------------------------------------*/


/* the buffer id numbers are not predefined when using the new 'by_name'*/
/* functions, in the past the following would be defined in a309.h      */
/* when an algorithm was integrated into the operational system         */
/* #define SAMPLE4_IP1        1998  */
/* #define SAMPLE4_IP2        1997  */


  
#endif

