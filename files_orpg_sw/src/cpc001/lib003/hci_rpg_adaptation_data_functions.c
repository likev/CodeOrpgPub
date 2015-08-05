/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:23 $
 * $Id: hci_rpg_adaptation_data_functions.c,v 1.2 2009/02/27 22:26:23 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_rpg_adaptation_data_functions.c			*
 *									*
 *	Description:  This module contains routines related to rpg	*
 *		      adaptation data.					*
 *									*
 ************************************************************************/

/* Local include file definition. */

#include <hci.h>
#include <hci_rpg_adaptation_data.h>

/***********************************************************************

    Rounds double value "v" in terms of "accu" and returns it with
    "vt". Returns 1, if no precision is lost, or 0 otherwise. If "v" is
    not a valid double, -1 is returned.

***********************************************************************/
int hci_rpg_adapt_str_accuracy_round (char *v, double accu, double *tv) {

    double dv, t, odv, diff;
    int negative, it;

    if (sscanf (v, "%lf", &dv) != 1)
        return (-1);

    if (tv != NULL)
        *tv = dv;
    if (accu <= 0.)
        return (1);
    odv = dv;
    negative = 0;
    if (dv < 0.) {
        dv = -dv;
        negative = 1;
    }
    t = dv / accu;
    it = (t + .5);
    dv = (double)it * accu;
    if (negative)
        dv = -dv;
    if (tv != NULL)
        *tv = dv;
    diff = dv - odv;
    if (diff < 0.)
        diff = -diff;
    if (dv < 0.)
        dv = -dv;
    if (diff < dv * .00000000001)
        return (1);
    return (0);

}

/***********************************************************************

    Rounds double value "dv" in terms of "accu" and returns it with
    "vt". Returns 1, if no precision is lost, or 0 otherwise. 

***********************************************************************/
int hci_rpg_adapt_accuracy_round (double dv, double accu, double *tv) {

    double t, odv, diff;
    int negative, it;

    if (tv != NULL)
        *tv = dv;
    if (accu <= 0.)
        return (1);
    odv = dv;
    negative = 0;
    if (dv < 0.) {
        dv = -dv;
        negative = 1;
    }
    t = dv / accu;
    it = (t + .5);
    dv = (double)it * accu;
    if (negative)
        dv = -dv;
    if (tv != NULL)
        *tv = dv;
    diff = dv - odv;
    if (diff < 0.)
        diff = -diff;
    if (dv < 0.)
        dv = -dv;
    if (diff < dv * .00000000001)
        return (1);
    return (0);

}

