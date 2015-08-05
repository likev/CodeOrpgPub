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
Module:         sample3_t2_adapt.h

Description:    adaptation definition file for the sample3_t2 demonstration 
                program. 
                
Authors:        Andy Stern, Software Engineer, Noblis Inc.
                    astern@noblis.org
                Tom Ganger, Systems Engineer,  Noblis Inc.
                    tganger@noblis.org
                Version 1.1, April 2005
                
Version 1.6   February 2008    T. Ganger
              Replaced C++ style comments using '//' for ANSI compliance.
                
$Id$
************************************************************************/

#ifndef _SAMPLE3_T2_ADAPT_H_
#define _SAMPLE3_T2_ADAPT_H_


/*typedef struct sample3_t2_adapt */
typedef struct
{
   
   int elev_select;        /*# @name "Elevation Index Selection"
                               @desc "Elev Index Used to Direct Sample 3 Final Product"
                               @min 1 @max 6 @default 1
                           */

} sample3_t2_adapt_t;



#endif




