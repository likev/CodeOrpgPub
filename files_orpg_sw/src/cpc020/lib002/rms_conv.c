/**************************************************************************
   
   Module: rms_conv.c 
   
   Description:
   This is the main module for converting 16 bit bytes into 
   integer  or real format.

   Assumptions:
      
   **************************************************************************/
/*
 * RCS info
 * $Author: dan $
 * $Locker:  $
 * $Date: 2001/01/11 19:35:38 $
 * $Id: rms_conv.c,v 1.2 2001/01/11 19:35:38 dan Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/*
* System Include Files/Local Include Files
*/

#include <math.h>
#include <stdio.h>
#include <rms_util.h>
#include <time.h>

/*
* Constant Definitions/Macro Definitions/Type Definitions
*/

#define BYTE_4 	4	/* Constant for incrementing pointer */	
#define BYTE_2 	2	/* Constant for incrementing pointer */	

/*
* Static Globals
*/

/*
* Static Function Prototypes
*/

/**************************************************************************

    Description: The following routines convert unsigned bytes into specified 
    formats. 

    Inputs:  buf_ptr - A pointer to the location of the byte to be converted.	

    Outputs:	
    
    Returns:  The specified format required by the calling routine.
**************************************************************************/

/* Convert buffer contents to signed 32 bit integer */
int conv_intgr (UNSIGNED_BYTE *buf_ptr) {

	int sgnd_32 =0;	/* Signed 32 bit intefer */
	
	memcpy (&sgnd_32, buf_ptr, BYTE_4);
	
	return (sgnd_32);
		
} /* End of convert integer */

 /* Convert buffer contents to unsigned 16 bit short */
ushort conv_ushrt (UNSIGNED_BYTE *buf_ptr) {

	ushort unsgnd_16 =0;	/* Unsigned 16 bit short */

        	memcpy (&unsgnd_16, buf_ptr, BYTE_2);
	return (unsgnd_16);

} /* End of convert unsigned short */

 /* Convert buffer contents to signed 16 bit short */
short conv_shrt (UNSIGNED_BYTE *buf_ptr) {

	short sgnd_16;	/* Signed 16 bit short */
	
	memcpy (&sgnd_16, buf_ptr, BYTE_2);

        	return (sgnd_16);

} /* End of convert signed short */

/* Convert buffer contents to a character */
void conv_char (UNSIGNED_BYTE *buf_ptr, char *temp_char, int num_bytes) {

	memcpy (temp_char, buf_ptr, num_bytes);
	
} /* End of convert character */

 /* Convert buffer contents to 32 bit float */
float conv_real (UNSIGNED_BYTE *buf_ptr) {


	float flt_32 =0;	/* 32 bit float */

        	memcpy (&flt_32, buf_ptr, BYTE_4);

	return (flt_32);
	
} /* End of convert real */

/**************************************************************************

    Description: The following routines convert specified formats into 
    unsigned bytes for placement into a buffer for transferring to the port 
    manager.  This routineplaces the converted object directly into the 
    buffer at a location pointed to by buf_ptr.

    Inputs:  buf_ptr - Pointer to a location in the output buffer.	
	     num - the number to be converted.
	     
    Outputs: 	
    
**************************************************************************/
/*Convert integer to unsigned byte*/
void conv_int_unsigned(UNSIGNED_BYTE *buf_ptr,int *num) {

	memcpy(buf_ptr, num, BYTE_4);

        	} /*End of convert integer to unsigned byte*/

/*Convert unsigned short to unsigned byte*/
void conv_ushort_unsigned(UNSIGNED_BYTE *buf_ptr,ushort *num) {

	memcpy(buf_ptr, num, BYTE_2);
	
	} /*End of convert unsigned short to unsigned byte*/
	
/*Convert short to unsigned byte*/
void conv_short_unsigned(UNSIGNED_BYTE *buf_ptr,short *num) {

	memcpy(buf_ptr, num, BYTE_2);

        	} /*End of convert short to unsigned byte*/

/* Convert character to unsigned byte*/
void conv_char_unsigned (UNSIGNED_BYTE *buf_ptr, char *temp_char, int num_bytes) {

		
	memcpy (buf_ptr,temp_char, num_bytes);

} /* End of convert character to unsigned byte*/

/*Convert float to unsigned byte*/
void conv_real_unsigned(UNSIGNED_BYTE *buf_ptr, float *in_flt) {

	
	memcpy(buf_ptr, in_flt, BYTE_4);
				
	} /*End of convert float to unsigned byte*/



/**************************************************************************

    Description: The following routine initializes a buffer to nulls.

    Inputs:	in_buf - Pointer to the buffer to be initialized.

    Outputs:	
    
**************************************************************************/	

	
void init_buf(UNSIGNED_BYTE *in_buf) {

	int i;
	char null = '\0';
	
	for (i=0; i<=MAX_BUF_SIZE; i++) {
		
		in_buf[i] = (UNSIGNED_BYTE) null;
		}
		
	} /* End init_buf*/	

	
/**************************************************************************

    Description: The following routine swaps bytes in a buffer.  This is 
    required for little endian machines.  The RMMS interfaces always sends
    and receives messages LSB first.

    Inputs:	in_buf - Pointer to the buffer to be swapped.
    		msg_size - number of bytes to swap.

    Outputs:	
    
**************************************************************************/	

	
int swap_bytes(UNSIGNED_BYTE *in_buf, int msg_size) {

/* This routine is only to be used on a little endian machine. */

#ifdef LITTLE_ENDIAN_MACHINE	
	int i;
	UNSIGNED_BYTE temp_buf;
	
	/* Swap the bytes in the buffer */
	
	for (i=0; i<msg_size; i = i+2){
		temp_buf = in_buf[i];
		in_buf[i] = in_buf[i+1];
		in_buf[i+1] = temp_buf;
		}
	
#endif	
		
	return (1);
		
	} /* End swap_bytes*/	


