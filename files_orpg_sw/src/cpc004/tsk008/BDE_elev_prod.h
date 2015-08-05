/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/09/18 16:33:11 $
 * $Id: BDE_elev_prod.h,v 1.2 2006/09/18 16:33:11 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef ELEV_PROD_H
#define ELEV_PROD_H

#include <basedata_elev.h>
#include <basedata.h>
#include <rpgc.h>
#include <orpg.h>
#include <a309.h>

/* Macro definitions. */
#define BDE_HEADER_SIZE			(sizeof(Compact_basedata_elev)-sizeof(Compact_radial))
#define MAX_PRODUCT_SIZE 		(BDE_HEADER_SIZE+(SR_MAX_RADIALS_ELEV*sizeof(Compact_radial)))

/* Define function prototypes. */
void BDE_build_product( void *input, void *output, int *size );


#endif
