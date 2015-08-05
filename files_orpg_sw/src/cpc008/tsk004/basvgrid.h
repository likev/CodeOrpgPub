/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/02/02 19:17:40 $ */
/* $Id: basvgrid.h,v 1.1 2007/02/02 19:17:40 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#ifndef BASVGRID_H
#define BASVGRID_H

#include <rpgc.h>
#include <rpgcs_miscellaneous.h>
#include <a308buf.h>

#define DTR		((float) DEGTORAD)
#define RDRNGF		1

typedef struct a308c4 {

   int colfact;

   int rowfact;

   float colel;

   float rowel;

} A308c4_t;


A308c4_t A308c4;

/* Function Prototypes. */
void Basvgrid_buffer_control( void );

#endif
