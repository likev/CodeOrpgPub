/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/04/05 15:16:48 $
 * $Id: orpgrda_adpt.h,v 1.6 2012/04/05 15:16:48 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef ORPGRDA_ADPT_H
#define ORPGRDA_ADPT_H


/* System Include Files/Local Include Files */
#include <orda_adpt.h>      

/* Macro definitions for RPG commonly used adaptation data items.
   NOTE: If adding new items, need to add support to ORPGRDA_APDT_get_values() */
#define ORPGRDA_ADPT_DELTA_PRI          1
#define ORPGRDA_ADPT_NBR_EL_SEGMENTS    2
#define ORPGRDA_ADPT_SEG1LIM            3
#define ORPGRDA_ADPT_SEG2LIM            4
#define ORPGRDA_ADPT_SEG3LIM            5
#define ORPGRDA_ADPT_SEG4LIM            6
#define ORPGRDA_ADPT_VELTOVER		7
#define ORPGRDA_ADPT_SPWTOVER		8
#define ORPGRDA_ADPT_REFTOVER		9

/* Function prototypes. */
void* ORPGRDA_ADPT_read();
void* ORPGRDA_ADPT_get_data();
int ORPGRDA_ADPT_get_data_value( int item, void *value );

#endif /*DO NOT REMOVE!*/
