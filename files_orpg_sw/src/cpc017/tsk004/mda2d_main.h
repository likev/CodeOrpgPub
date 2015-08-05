/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2005/03/03 22:19:16 $
 * $Id: mda2d_main.h,v 1.4 2005/03/03 22:19:16 ryans Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda2d_main.h

Description:    include file for the MDA 2D algorithm
                mda2d_acl.c. This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
************************************************************************/

#ifndef MDA2D_MAIN_H 
#define MDA2D_MAIN_H 

/* system includes ---------------------------------------------------- */
#include <stdio.h>

/* ORPG   includes ---------------------------------------------------- */
#include <rpg_globals.h>
#include <rpgc.h>
#include <basedata.h>
#include <alg_adapt.h>

/* Algorithm specifid constants/definitions --------------------------- */
                            
#define BUFSIZE 50000     /* estimated output size for the ICD product */
                            /* based on 6 degree azimuth and 5 km range */
                            /* and 20 elevations per volume, the maximum*/
                            /* range is 120km   			*/

/* declare the modules */
void mda2d_acl();
void mda2d_set_table();

#endif


