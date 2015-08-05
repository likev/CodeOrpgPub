/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:32 $
 * $Id: byteswap.c,v 1.5 2009/05/15 17:52:32 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* functions with the purpose of helping the byteswapping for endian issues */

#include "byteswap.h"





/****************************************************************

   Description:
      Packs 4 bytes pointed to by "value" into 2 unsigned shorts.
      "value" can be of any type.  The address where the 4 bytes
      starting at "value" will be stored starts @ "loc".  

      The Most Significant 2 bytes (MSW)  of value are stored at 
      the byte addressed by "loc", the Least Significant 2 bytes 
      (LSW) are stored at 2 bytes past "loc".  

      By definition:
     
         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff ) 
 
   Input:
      loc - starting address where to store value. 
      value - pointer to data value.

   Output:
      loc - stores the MSW halfword of "value" at
            (unsigned short *) loc and the LSW halfword of
            "value" at ((unsigned short *) loc) + 1.

   Returns:
      Always returns 0.

****************************************************************/
/* int MISC_pack_ushorts_with_value( void *loc, void *value ){ */
int write_orpg_product_int( void *loc, void *value ){

   unsigned int   fw_value = *((unsigned int *) value);
   unsigned short hw_value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   hw_value = (unsigned short) (fw_value >> 16) & 0xffff;
   *msw = hw_value;

   hw_value = (unsigned short) (fw_value & 0xffff);
   *lsw = hw_value;

   return 0;

/* End of write_orpg_product_int() */
}
 
/****************************************************************

   Description:
      Unpacks the data value @ loc.  The unpacked value will be 
      stored at "value". 

      The Most Significant 2 bytes (MSW) of the packed value are
      stored at the byte addressed by "loc", the Least Significant
      2 bytes (LSW) are stored at 2 bytes past "loc".  

      By definition:
     
         MSW = ( 0xffff0000 & (value << 16 ))
         LSW = ( value & 0xffff ) 
 
   Input:
      loc - starting address where packed value is stored.
      value - address to received the packed value.

   Output:
      value - holds the unpacked value.

   Returns:
      Always returns 0.

****************************************************************/
/* int MISC_unpack_value_from_ushorts( void *loc, void *value ){ */
int read_orpg_product_int( void *loc, void *value ){
   unsigned int *fw_value = (unsigned int *) value;
   unsigned short *msw = (unsigned short *) loc;
   unsigned short *lsw = msw + 1;

   *fw_value = 
      (unsigned int) (0xffff0000 & ((*msw) << 16)) | ((*lsw) & 0xffff);

   return 0;

/* End of read_orpg_product_int() */
}



int write_orpg_product_float( void *loc, void *value )
{
int ret;
    
    ret = write_orpg_product_int( loc, value );
    
    return ret;
    
}




int read_orpg_product_float( void *loc, void *value )
{
int ret;

    ret = read_orpg_product_int( loc, value );
    
    return ret;
    
}



