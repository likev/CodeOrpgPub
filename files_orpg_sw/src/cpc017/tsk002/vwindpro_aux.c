/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2010/10/18 14:37:41 $
 * $Id: vwindpro_aux.c,v 1.2 2010/10/18 14:37:41 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <vwindpro.h>

/*\//////////////////////////////////////////////////////////

   Description:
      Performs complex absolute value. 

   Inputs:
      z1 - the number to for which we want absolute value.
     
   Outputs:
      c - the result of complex absolute value.

//////////////////////////////////////////////////////////\*/   
void Cmplx_abs( Complex_t *z, double *c ){

   double s = (z->real*z->real) + (z->img*z->img);
   *c = sqrt(s);
 
} /* End of Cmplx_abs(). */

/*\//////////////////////////////////////////////////////////

   Description:
      Performs complex multiplication. 

   Inputs:
      z1 - the first number to be multiplied.
      z2 - the second number to be multiplied.
     
   Outputs:
      c - the result of the complex multipication.

//////////////////////////////////////////////////////////\*/   
void Cmplx_mult( Complex_t *z1, Complex_t *z2, Complex_t *c ){

    c->real = (z1->real*z2->real)  -  (z1->img*z2->img);
    c->img = (z1->real*z2->img)  +  (z1->img*z2->real);

} /*End of Cmplx_mult(). */


/*\/////////////////////////////////////////////////////////

   Description:
      Performs complex multiplication.

   Inputs:
      z1 - the dividend
      z2 - the divisor

   Outputs:
      c - The results of the complex division.

/////////////////////////////////////////////////////////\*/
void Cmplx_div( Complex_t *z1, Complex_t *z2, Complex_t *c ){

    double y, p, q;

    /* z1 = a + bi, z2 = c + di  where

       z1/z2 = ((ac + bd) + i(bc - ad)) / (c^2 + d^2) */
    y = (z2->real*z2->real)  +  (z2->img*z2->img);
    p = (z1->real*z2->real)  +  (z1->img*z2->img);
    q = (z1->img*z2->real)  -  (z1->real*z2->img);

    if( y == 0.0 ){

        c->real = 0.0;
        c->img = 0.0;

    }
    else{

        c->real = p/y;
        c->img = q/y;

    }

} /* End of Cmplx_div(). */
