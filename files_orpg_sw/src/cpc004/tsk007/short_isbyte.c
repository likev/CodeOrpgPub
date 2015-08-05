/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/11/26 21:43:54 $
 * $Id: short_isbyte.c,v 1.2 2002/11/26 21:43:54 nolitam Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/************************************************************************
Module:         short_isbyte.c             

Description:    This module contains a translation of the FORTRAN       
                function ISBYTE. Some differences the translation were
                required due to type casting between I*4 integers and
                ANSI-C short (I*2) integers.
                
Authors:        Andy Stern, Software Engineer, Mitretek Systems
                    astern@mitretek.org
                Tom Ganger, Systems Engineer,  Mitretek Systems
                    tganger@mitretek.org
                Version 1.0, October 2000
$Id: short_isbyte.c,v 1.2 2002/11/26 21:43:54 nolitam Exp $
************************************************************************/

#include "short_isbyte.h"

/* Technical Discussion of FORTRAN VII ISBYTE ----------------------------

   Implements the FORTRAN VII built-in Byte Operation subroutine ISBYTE
   in ANSI C

   Description:
      ISBYTE stores a byte from the low order position of one argument
      to any arbitrary position in another argument

      The format of the FORTRAN subroutine call is:
         CALL ISBYTE(k,m,n)
      where
         k = is the integer input argument whose least significant byte
             is to be stored.
         m = is the integer output argument into which a byte is stored.
         n = is the integer number of the byte in m where storing occurs.

       The effect of the call is:

            m               <- k
              8n - (8n + 7)      24 - 31

       Hence, if n=0, the LSB of k will occupy bytes  0 -  7 (MSB) in m.
              if n=1, the LSB of k will occupy bytes  8 - 15 (byte 2) in m.
              if n=2, the LSB of k will occupy bytes 16 - 23 (byte 1) in m.
              if n=3, the LSB of k will occupy bytes 24 - 31 (byte 0) in m.
              Bytes are numbered from zero at the high order byte of a
              variable.
                                  ___________________________
                                 |______|______|______|______|
                       byte pos   0    7 8   15 16  23 24  31
                       byte #        3      2      1      0

       Note: All arguments must be INTEGER*n. When n=0, the least
       significant byte of k is stored into the most significant byte
       of m. Arguments k and n are not altered by this routine. Only
       the nth byte of m is changed (all other bytes of m are unchanged).

   Functionality:
      In the following example, the call to ISBYTE takes the least
      significant byte from X (byte 0) and stores it into the second
      byte (byte 1) of Integer Y:        ISBYTE(X,Y,2)

            Integer X                              Integer Y
       ___________________                     ___________________
      |____|____|____|____|                   |____|____|____|____|
         3   2    1    0                         3   2    1    0
                        |                                 ^
                        |_________________________________|

      If n==0, then byte 0 (LSB) of X is stored into byte 3 (MSB) of Y.

   C Prototype:

   unsigned int isbyte(int input_word,int store_position)

      where input_word = the integer input argument whose least
                         significant byte is to be stored.
            store_position = is the integer number of the byte
                         where storing occurs.
            function returns 'm', an integer with the stored byte.

      Note: The input arguments are NOT altered by this routine.

            -----------------------------------------------------    
            
   IMPORTANT NOTE: THIS VERSION OF ISBYTE DEALS WITH SHORT INPUT
   AND OUTPUT PARAMETERS. HENCE THIS VERSION (SHORT_ISBYTE) ONLY
   DEALS WITH 2-BYTE INTEGERS            
            
   ---------------------------------------------------------------------*/

/************************************************************************
Description:    modified version of ISBYTE which uses short integer
                input and output parameters
Input/Output:   short input_word   (Const Input) Supplies the byte to be
                   stored within the output word. The byte comes from
                   the LSB (least significant byte) of the word.
                short *output_word (In/Out) Receives the byte from input 
                   at the byte position specified by store_position.
                int store_position (Const Input) Supplies the byte 
                   position where the LSB of input_word will be stuffed 
                   within output_word.
Returns:        The function return value is TRUE upon success or FALSE 
                upon failure.
Globals:        none
Notes:          the program is still being validated
                There are two bytes in a short integer, the LEFT byte
                (n=0) and the RIGHT byte (n=1). These values are 
                indicated in the store_position parameter. Any other 
                value input in this parameter will yield an error return.
************************************************************************/
int short_isbyte(short input_word, short *output_word, int store_position) {

   /* Local Variables --------------------------------------------------*/
   int TEST=FALSE;                  /* Set to TRUE to show diagnostics  */
   unsigned short LSB=0;           /* holds the LSB of the input word   */
   unsigned short value=0;         /* temporary use variable            */
   unsigned short lsb_mask=0xFF;   /* mask to access lowest sgfnt byte  */
   unsigned short shift_value=0;   /* calculated left shift value       */
   char *byte[]={"Left","Right"};

   /* Input Instructions:
        if store_position == 0 then place the LSB into LEFT Byte
        if store_position == 1 then place the LSB into the RIGHT Byte   */

   /* array that holds 2 masks. each mask is used on a 2 byte word      */
   /* to isolated one particular byte (when bitwise AND with input)     */
   unsigned short mask[]={0x00FF,0xFF00};

   /* Quality Control: shift_position must be 0>= x <=1 otherwise       */
   /* return an error flag (FALSE). A good return is TRUE.              */
   if(store_position<0 || store_position>1) {
      fprintf(stderr,"short_ISBYTE input error: parameter 3 is out of bounds\n");
      return(FALSE);
      }

   if(TEST==TRUE) {
      fprintf(stderr,"\n------------------------------------------------\n");
      fprintf(stderr,"\nshort ISBYTE Test Diagnostics (before processing)\n");
      fprintf(stderr,"   input_word       =%4u or 0x%02x\n",input_word,input_word);
      fprintf(stderr,"   initial outputwrd=%4u or 0x%02x\n",*output_word,
         *output_word);
      fprintf(stderr,"   store_position   =%4u or %s\n",store_position,
        byte[store_position]);
      }

   /* calculate the bitwise left shift value using the store_position   */
   shift_value = (short)(8 - (store_position * 8));

   /* obtain the LSB (lowest significant byte) of the 2 byte word       */
   LSB = (short)(input_word & lsb_mask);

   /* clear the byte that will be stuffed with the new value            */
   *output_word=(short)(*output_word & mask[store_position]);

   /* shift the LSB left according to the calculated shift value        */
   value=(short)(LSB << shift_value);

   /* stuff the new value into the output using a bitwise OR            */
   *output_word=(short)(*output_word | value);

   if(TEST==TRUE) {
      fprintf(stderr,"\nshort ISBYTE Test Diagnostics (after Processing)\n");
      fprintf(stderr,"   left shift value =%4u\n",shift_value);
      fprintf(stderr,"   input LSB:       =%4u or 0x%04x\n",LSB,LSB);
      fprintf(stderr,"   after shift left =%4u or 0x%04x\n",value,value);
      fprintf(stderr,"   final output word=%4u or 0x%04x\n",*output_word,
         *output_word);
      }

   return(TRUE);
   } /* end of function isbyte */

