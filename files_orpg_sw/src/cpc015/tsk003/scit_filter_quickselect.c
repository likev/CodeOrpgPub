/*
 * RCS info
 * $Author: ryans $
 * $Date: 2005/02/23 22:30:34 $
 * $Locker:  $
 * $Id: scit_filter_quickselect.c,v 1.1 2005/02/23 22:30:34 ryans Exp $
 * $revision$
 * $state$
 * $Logs$
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *      Module:         quickselect.c                                         *
 *      Author:         Yukuan Song                                           *
 *      Created:        January 28, 2005                                      *
 *      References:     This Quickselect routine is based on the algorithm    *
 *                      described in "numerical recipies in C", Second        *
 *                      edition , Cambridge University Press, 1992,           *
 *                      section 8.5, ISBN 0-521-43108-5                       *
 *                                                                            *
 *      Description:    This is one of the most efficient methods to find     *
 *                      the middle value of an one-dimension array.           *
 *                      It is efficient because the middle value can be       *
 *                      identified without completely sorting the array       *
 *                                                                            *
 *      Input:          arr[]: one-dimension array                            *
 *                      n:     number of the elements in the array            *
 *      Output:         middle value of the input array                       *
 *      returns:        middle value of the input array                       *
 *      Notes:                                                                *
 ******************************************************************************/

#define elem_type short 
#define ELEM_SWAP(a,b) { register elem_type t=(a);(a)=(b);(b)=t; }

elem_type quick_select(elem_type arr[], int n) 
{
    int low, high ;
    int median;
    int middle, ll, hh;

    low = 0 ; high = n-1 ; median = (low + high) / 2;
    for (;;) {
        if (high <= low) /* One element only */
            return arr[median] ;

        if (high == low + 1) {  /* Two elements only */
            if (arr[low] > arr[high])
                ELEM_SWAP(arr[low], arr[high]) ;
            return arr[median] ;
        }

    /* Find median of low, middle and high items; swap into position low */
    middle = (low + high) / 2;
    if (arr[middle] > arr[high])    ELEM_SWAP(arr[middle], arr[high]) ;
    if (arr[low] > arr[high])       ELEM_SWAP(arr[low], arr[high]) ;
    if (arr[middle] > arr[low])     ELEM_SWAP(arr[middle], arr[low]) ;

    /* Swap low item (now in position middle) into position (low+1) */
    ELEM_SWAP(arr[middle], arr[low+1]) ;

    /* Nibble from each end towards middle, swapping items when stuck */
    ll = low + 1;
    hh = high;
    for (;;) {
        do ll++; while (arr[low] > arr[ll]) ;
        do hh--; while (arr[hh]  > arr[low]) ;

        if (hh < ll)
        break;

        ELEM_SWAP(arr[ll], arr[hh]) ;
    }

    /* Swap middle item (in position low) back into correct position */
    ELEM_SWAP(arr[low], arr[hh]) ;

    /* Re-set active partition */
    if (hh <= median)
        low = ll;
        if (hh >= median)
        high = hh - 1;
    } /* END of "for (;;) */
}

#undef ELEM_SWAP
