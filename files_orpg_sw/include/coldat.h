/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/05/23 19:40:06 $
 * $Id: coldat.h,v 1.3 2006/05/23 19:40:06 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef COLDAT_H
#define COLDAT_H

#define PCTTHSIZ 	16
#define PCTTHMAX  	34
#define PCTCDMAX  	257

/* Note: For coldat, the data levels vary from 0 to 256 so
         we do not have to adjust index to account for C
         versus FORTRAN. */
typedef struct coldat {

   int color_table_first;

   short thresh[ PCTTHMAX ][ PCTTHSIZ ];

   short coldat[ PCTTHMAX ][ PCTCDMAX ];

   int color_table_last; 

} Coldat_t;

# endif
