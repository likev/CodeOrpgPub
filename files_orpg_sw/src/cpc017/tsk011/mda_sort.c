/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/01/07 17:57:34 $
 * $Id: mda_sort.c,v 1.3 2004/01/07 17:57:34 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         mda_sort.c					      *
 *	Author:		Brian Klein					      *
 *   	Created:	Feb. 19, 2003					      *
 *	References:	WDSS MDA Fortran Source code (mdatrck.f dated 121602) *
 *			AEL						      *
 *									      *
 *      Description:    This file contains a sorting routine taken from the   *
 *                      NSSL MDA Tracking algorithm.                          *
 *									      *
 *      Notes:       	Original NSSL Fortran code comments have been         *
 *                      retained.                                             *
 ******************************************************************************/

#include "mdattnn.h"

void mda_sort(int sort_flag, cplt_t cplt[], int nbr)
{
        short i, j, k;
        float   tj, tk;
        cplt_t temp;

        tj = 0;
        tk = 0;

/*C******************************************/
/*C  Sort the mesos:                       **/
/*C    0:  by strength rank then meso type **/
/*C    1:  by strength rank then MSI       **/
/*C******************************************/

        for (i = 0; i < nbr - 1; i++) {

          for (j = 0; j < nbr - i -1; j++) {

            k = j + 1;

            if (sort_flag == SORT_BY_RANK_THEN_TYPE) {
              tj = (cplt[j].strength_rank * 10) + cplt[j].circ_class;
/****BUG!!!*/
/***              tk = (cplt[j].strength_rank * 10) + cplt[k].circ_class;***/
/***Correct line!!!***/
              tk = (cplt[k].strength_rank * 10) + cplt[k].circ_class;
            } else {
              tj = (cplt[j].strength_rank * 100000.) + cplt[j].msi;
              tk = (cplt[k].strength_rank * 100000.) + cplt[k].msi;
            } /* end if */

            if (tj < tk) {
                temp = cplt[k];
                cplt[k] = cplt[j];
                cplt[j] = temp;
            }

          } /* end for j loop */

        } /* end for i loop */

/*C*****************************/
/*C  End of program unit body **/
/*C*****************************/

        return;
} /* end of function mda_sort() */

