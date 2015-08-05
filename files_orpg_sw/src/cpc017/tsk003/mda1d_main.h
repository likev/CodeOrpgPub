/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/19 19:55:58 $
 * $Id: mda1d_main.h,v 1.7 2009/03/19 19:55:58 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/************************************************************************
Module:         mda1d_main.h

Description:    include file for the MDA 1D algorithm
                mda1d_main.c. This file contains module 
                specific include file listings, function prototypes 
                and constant definitions.
Author: Yukuan Song
Created: August 2002
************************************************************************/

#ifndef MDA1D_MAIN_H 
#define MDA1D_MAIN_H 

/* system includes ---------------------------------------------------- */
#include <stdio.h>

/* ORPG   includes ---------------------------------------------------- */
#include <rpg_globals.h>
#include <rpgc.h>
#include <basedata.h>
#include <alg_adapt.h>

/* Algorithm specifid constants/definitions --------------------------- */
                            
#define BUFSIZE 600000     /* estimated output linear buffer size */

/* Function prototypes  */
void mda1d_acl();

#endif


