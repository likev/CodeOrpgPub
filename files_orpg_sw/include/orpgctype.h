/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:35 $
 * $Id: orpgctype.h,v 1.12 2002/12/11 22:10:35 nolitam Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
/**************************************************************************

      Module: orpgctype.h

 Description: Global Open Systems Radar Product Generator (ORPG) data
              types header file.
       Notes:
 **************************************************************************/

#ifndef ORPGCTYPE_H
#define ORPGCTYPE_H


/*
 * The following typedef's are to be used (1) in argument lists of C routines
 * that are called by Fortran routines and (2) in C data structures that map
 * to Fortran data structures (e.g., C structure -> Fortran common block)
 */
typedef int fint ;
typedef short fint2 ;
typedef int fint4 ;

typedef int flogical ;
typedef short flogical2 ;
typedef int flogical4 ;

typedef float freal ;

#endif /*DO NOT REMOVE!*/
