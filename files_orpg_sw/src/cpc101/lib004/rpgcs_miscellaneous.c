/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/06/18 18:23:19 $
 * $Id: rpgcs_miscellaneous.c,v 1.6 2007/06/18 18:23:19 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#include <rpgcs_miscellaneous.h>

/**************************************************************************
   Description:
      This function test a bit for 4-byte data.  The bit offset is
      not limited to testing bits within a four-byte word.

   Input:
      data - pointer to 4-byte data
      off -  bit offset

   Output:
      The specified bit is test.

   Returns:
      Returns -1 on error, 0 is bit is not set, 1 if bit is set.

   Notes:
      This function accomodates Big Endian and Little Endian architectures.
      The legacy software was written for a Big Endian architecture, so the
      bit offset will be a Big Endian bit offset.  We simply "reshuffle"
      the calculated byte offset for the Little Endian architecture:

      Big Endian:  ByteN+3 ByteN+2 ByteN+1 ByteN
      Little Endian:  ByteN   ByteN+1 ByteN+2 ByteN+3

**************************************************************************/
int RPGCS_bit_test( unsigned char *data, int off ){

    int byte_offset;
    int bit_offset;

    byte_offset = off / RPGCS_CHAR_BIT;

#ifdef LITTLE_ENDIAN_MACHINE
    if( (byte_offset % 4) == 0 )
        byte_offset = byte_offset + 3;

    else if( (byte_offset % 4) == 1 )
        byte_offset = byte_offset + 1;

    else if( (byte_offset % 4) == 2 )
        byte_offset = byte_offset - 1;

    else
        byte_offset = byte_offset - 3;

#endif

    /* Leftmost bit is 0 bit .... offset of 3 bits from the left
       translates to a left bit shift of four ... */
    bit_offset = (RPGCS_CHAR_BIT - 1) - (off % RPGCS_CHAR_BIT);
    if( data[byte_offset] & (1 << bit_offset) )
       return 1;

    return 0;

/* End of RPGCS_bit_test() */
}
/**************************************************************************
   Description:
      This function test a bit for 2-byte data.  The bit offset is
      not limited to testing bits within a two-byte word.

   Input:
      data - pointer to 2-byte data
      off -  bit offset

   Output:
      The specified bit is test.

   Returns:
      Returns -1 on error, 0 is bit is not set, 1 if bit is set.

   Notes:
      This function accomodates Big Endian and Little Endian architectures.
      The legacy software was written for a Big Endian architecture, so the
      bit offset will be a Big Endian bit offset.  We simply "reshuffle"
      the calculated byte offset for the Little Endian architecture:

      Big Endian:  ByteN+1 ByteN
      Little Endian:  ByteN ByteN+1

**************************************************************************/
int RPGCS_bit_test_short( unsigned char *data, int off ){

    int byte_offset;
    int bit_offset;

    byte_offset = off / RPGCS_CHAR_BIT;

#ifdef LITTLE_ENDIAN_MACHINE
    if( byte_offset % 2 )
        byte_offset = byte_offset - 1;

    else
        byte_offset = byte_offset + 1;

#endif

    /* Leftmost bit is 0 bit .... offset of 3 bits from the left
       translates to a left bit shift of four ... */
    bit_offset = (RPGCS_CHAR_BIT - 1) - (off % RPGCS_CHAR_BIT);
    if( data[byte_offset] & (1 << bit_offset) )
       return 1;

    return 0;

/* End of RPGCS_bit_test_short() */
}


/**************************************************************************
   Description: 
      This function clears a bit for 4-byte data.  The bit offset is 
      not limited to clearing bits within a four-byte word.

   Input: 
      data - pointer to 4-byte data
      off -  bit offset

   Output: 
      The specified bit is cleared.

   Returns:  
      -1 on error, 0 otherwise.

   Notes: 
      This function accomodates Big Endian and Little Endian architectures.  
      The legacy software was written for a Big Endian architecture, so the 
      bit offset will be a Big Endian bit offset.  We simply "reshuffle" 
      the calculated byte offset for the Little Endian architecture:

      Big Endian:  ByteN+3 ByteN+2 ByteN+1 ByteN
      Little Endian:  ByteN   ByteN+1 ByteN+2 ByteN+3 

**************************************************************************/
int RPGCS_bit_clear( unsigned char *data, int off ){

   int byte_offset;
   int bit_offset;
 
   if( data == NULL )
      return -1;

   byte_offset = off / RPGCS_CHAR_BIT;

#ifdef LITTLE_ENDIAN_MACHINE
   if( (byte_offset % 4) == 0 ) 
       byte_offset = byte_offset + 3;
     
   else if( (byte_offset % 4) == 1 )
       byte_offset = byte_offset + 1;
    
   else if( (byte_offset % 4) == 2 )
       byte_offset = byte_offset - 1;
     
   else 
       byte_offset = byte_offset - 3;
    
#endif

   /* Leftmost bit is 0 bit .... offset of 3 bits from the left 
      translates to a left bit shift of four ... */
   bit_offset = (RPGCS_CHAR_BIT - 1) - (off % RPGCS_CHAR_BIT);
   data[byte_offset] &= ~(1 << bit_offset);

   return 0;

/* End of RPGCS_bit_clear() */
}

/**************************************************************************

   Description:
      This function clears a bit for 2-byte data.  The bit offset is
      not limited to clearing bits within a two-byte word.

   Input:
      data - pointer to 2-byte data
      off -  bit offset

   Output:
      The specified bit is cleared.

   Returns:
      -1 on error, 0 otherwise.

   Notes:
      This function accomodates Big Endian and Little Endian architectures.
      The legacy software was written for a Big Endian architecture, so the
      bit offset will be a Big Endian bit offset.  We simply "reshuffle"
      the calculated byte offset for the Little Endian architecture:

      Big Endian:  ByteN+1 ByteN
      Little Endian:  ByteN ByteN+1

**************************************************************************/
int RPGCS_bit_clear_short( unsigned char *data, int off ){

   int byte_offset;
   int bit_offset;
 
   if( data == NULL )
      return -1;

   byte_offset = off / RPGCS_CHAR_BIT;

#ifdef LITTLE_ENDIAN_MACHINE
   if( byte_offset % 2 )
       byte_offset = byte_offset - 1;
     
   else 
       byte_offset = byte_offset + 1;
    
#endif

   /* Leftmost bit is 0 bit ... offset of 3 bits from the left translates 
      to a left bit shift of four ... */
   bit_offset = (RPGCS_CHAR_BIT - 1) - (off % RPGCS_CHAR_BIT);
   data [byte_offset] &= ~(1 << bit_offset);

   return 0;

/* End of RPGCS_bit_clear_short() */
}

/**************************************************************************
   Description:
      This function sets a bit for 4-byte data.  The bit offset is
      not limited to setting bits within a four-byte word.

   Input:
      data - pointer to 4-byte data
      off -  bit offset

   Output:
      The specified bit is set.

   Returns:
      -1 on error, 0 otherwise.

   Notes:
      This function accomodates Big Endian and Little Endian architectures.
      The legacy software was written for a Big Endian architecture, so the
      bit offset will be a Big Endian bit offset.  We simply "reshuffle"
      the calculated byte offset for the Little Endian architecture:

      Big Endian:  ByteN+3 ByteN+2 ByteN+1 ByteN
      Little Endian:  ByteN   ByteN+1 ByteN+2 ByteN+3

**************************************************************************/
int RPGCS_bit_set( unsigned char *data, int off ){

   int byte_offset;
   int bit_offset;
 
   if( data == NULL )
      return -1;

   byte_offset = off / RPGCS_CHAR_BIT;

#ifdef LITTLE_ENDIAN_MACHINE
   if( (byte_offset % 4) == 0 ) 
       byte_offset = byte_offset + 3;
    
   else if( (byte_offset % 4) == 1 )
       byte_offset = byte_offset + 1;
    
   else if( (byte_offset % 4) == 2 ) 
       byte_offset = byte_offset - 1;
    
   else 
       byte_offset = byte_offset - 3;
    
#endif

   /* Leftmost bit is 0 bit ... offset of 3 bits from the left translates 
      to a left bit shift of four ... */
   bit_offset = (RPGCS_CHAR_BIT - 1) - (off % RPGCS_CHAR_BIT) ;
   data [byte_offset] |= (1 << bit_offset);

   return (0);

/* End of RPGCS_bit_set() */
}

/**************************************************************************

   Description:
      This function sets a bit for 2-byte data.  The bit offset is
      not limited to setting bits within a two-byte word.

   Input:
      data - pointer to 2-byte data
      off -  bit offset

   Output:
      The specified bit is set.

   Returns:
      There is no return value defined for this function.   Always returns
      0.

   Notes:
      This function accomodates Big Endian and Little Endian architectures.
      The legacy software was written for a Big Endian architecture, so the
      bit offset will be a Big Endian bit offset.  We simply "reshuffle"
      the calculated byte offset for the Little Endian architecture:

      Big Endian:  ByteN+1 ByteN
      Little Endian:  ByteN ByteN+1

**************************************************************************/
int RPGCS_bit_set_short( unsigned char *data, int off ){

   int byte_offset;
   int bit_offset;
 
   if( data == NULL )
      return -1;

   byte_offset = off / RPGCS_CHAR_BIT ;

#ifdef LITTLE_ENDIAN_MACHINE
   if( byte_offset % 2 ) 
       byte_offset = byte_offset - 1;
    
   else 
       byte_offset = byte_offset + 1;
    
#endif

   /* Leftmost bit is 0 bit ... offset of 3 bits from the left translates 
      to a left bit shift of four ... */
   bit_offset = (RPGCS_CHAR_BIT - 1) - (off % RPGCS_CHAR_BIT) ;
   data[byte_offset] |= (1 << bit_offset);

   return (0);

/* End of RPGCS_bit_set_short() */
}

#define LIST_HEADER 	2	/* list header size in number of ints */

/*********************************************************************

   Description: 
      The following defines a circular list.

   Inputs:
      list - The list array;
      size - list size;

   Output:
      list - Initialized list array;

   Return:
      The return value is meaningless. On failure this function calls 
      PS_task_abort to terminate the task.

   Notes: 
      The first two elements in the list array are used for list 
      control (header). list[0] is the size of the list (must be 
      <= 32767).  list[1] = tpt + (nitems << 16), where tpt is the 
      pointer pointing to the location of the "next" new top 
      element and nitems is the number of elements currently
      in the list.

*********************************************************************/
int RPGCS_define_clist( int *list, int size ){

   if( (size < 1) || (size > 32767) )
	PS_task_abort( "RPGCS_define_clist Size Error: (%d)\n", size );
    else
	list[0] = size;

    list[1] = 0;

    return (0);

/* End of RPGCS_define_clist() */
}

/*********************************************************************

   Description: 
      This functions adds value to top of a circular list.

   Inputs:
      value - value to add to the top of the list;
      list - The list array;

   Output:
      list - midified list array;

   Return:
      Returns -1 on error, 1 if the circular list is full and 0
      otherwise.

*********************************************************************/
int RPGCS_add_top_clist( int value, int *list ){

    int tpt, nitems, size;

    if( list == NULL )
       return -1;

    tpt = list[1] & 0xffff;
    nitems = (list[1] >> 16) & 0xffff;
    size = list[0];

    if( nitems >= size ) 
	return 1;

    list[tpt + LIST_HEADER] = value;
    tpt = (tpt + 1) % size;
    nitems++;

    list[1] = tpt + (nitems << 16);

    return 0;

/* End of RPGCS_add_top_clist() */
}

/*********************************************************************

   Description: 
      This function adds value to bottom of a circular list.

   Inputs:	
      value - value to add to the bottom of the list;
      list - The list array;

   Output:
      list - midified list array;

   Return:
      Returns -1 on error, 1 if the circular list is full or 0
      otherwise.

*********************************************************************/
int RPGCS_add_bottom_clist( int value, int *list ){

   int tpt, nitems, size;

   if( list == NULL )
      return -1;

   tpt = list[1] & 0xffff;
   nitems = (list[1] >> 16) & 0xffff;
   size = list[0];

   if( nitems >= size ) 
      return (1);
    

   nitems++;
   list[((tpt - nitems + size) % size) + LIST_HEADER] = value;

   list[1] = tpt + (nitems << 16);

   return 0;

/* End of RPGCS_add_bottom_clist() */
}


/*********************************************************************

   Description: 
      This function removes the top element of a list.

   Inputs:	
      list - The list array;

   Output:	
      value - value removed from the top of the list;
      list - midified list array;

   Return:
      -1 on error, 1 if item is removed but list is not empty
      after removal, 2 if list is initially empty, or 0 if i
      after item is removed, the list is empty.

*********************************************************************/
int RPGCS_remove_top_clist( int *list, int *value ){

   int tpt, nitems, size;

   if( list == NULL )
      return -1;

   tpt = list[1] & 0xffff;
   nitems = (list[1] >> 16) & 0xffff;
   size = list[0];

   if( nitems == 0 ) 
      return 2;

   tpt = (tpt - 1 + size) % size;
   *value = list[tpt + LIST_HEADER];
   nitems--;

   list[1] = tpt + (nitems << 16);

   if (nitems > 0)
	return 1;
    else
	return 0;

    return (0);

/* End of RPGCS_remove_top_clist() */
}

/*********************************************************************

   Description: 
      This function remove element from the bottom of a circular list.

   Inputs:	
      list - The list array;

   Output:	
      value - value removed from the bottom of the list;
      list - modified list array;

   Return:
      -1 on error, 1 is after removal the list is not empty,
      2 if the list is initially empty, or 0 is after removal
      the list is not empty.

*********************************************************************/
int RPGCS_remove_bottom_clist( int *list, int *value ){

   int tpt, nitems, size;

   if( list == NULL )
      return -1;

   tpt = list[1] & 0xffff;
   nitems = (list[1] >> 16) & 0xffff;
   size = list[0];

   if( nitems == 0 ) 
      return 2;

   *value = list[((tpt - nitems + size) % size) + LIST_HEADER];
   nitems--;

   list[1] = tpt + (nitems << 16);

   if( nitems > 0 )
      return 1;
   else
      return 0;

/* End of RPGC_remove_bottom_clist() */
}

/***************************************************************

   Description:
      Takes the Most Significant Short Word of "value" and
      returns as an unsigned integer.

***************************************************************/
unsigned int RPGC_set_mssw_to_uint( unsigned int value ){

   return( (unsigned int) (value >> 16) & 0xffff );

/* End of RPGCS_set_mssw_to_uint() */
}

#define MAX_FILENAME_CHARS              12
#define MAX_FULLNAME_CHARS             255

/******************************************************************

   Description: 
      Prefixes "temporary directory" path to file name.  The 
      temporary directory path is returned from MISC_get_work_dir.

   Inputs:
      file_name - Character array (up to 12 characters)

   Outputs:
      full_name - Character array (up to 255 characters).
                  Gives the fully quanified path for "file_name".

   Notes:

******************************************************************/
void RPGCS_full_name( char *file_name, char *full_name ){

   int i, num_chars, length_of_path;
   char work_dir[ MAX_FULLNAME_CHARS ]; 

   /* Initialize the work_dir array to zeros. */
   memset( work_dir, 0, MAX_FULLNAME_CHARS );

   /* The "file_name" is assumed to have no more than 12 characters. */
   num_chars = 0;
   while( (isalnum( (int) file_name[num_chars]) || (file_name[num_chars] == '.' ))
                         && 
          (num_chars < MAX_FILENAME_CHARS )){
      num_chars++;

   }

   /* Get the path length of the working directory. */
   length_of_path = MISC_get_work_dir( work_dir, MAX_FULLNAME_CHARS );
   if( length_of_path < 0 ){

      /* Some error ocurred.  Copy "file_name" to "full_name" */
      memcpy( full_name, file_name, num_chars );

      /* Pad name with blanks. */
      for( i = num_chars; i < MAX_FULLNAME_CHARS; i++ )
         full_name[i] = ' '; 
   
      return;

   }
   
   /* Prepend the "work_dir" to "file_name".   "work_dir" does not
      include the trailing '/' */
   memcpy( full_name, work_dir, length_of_path );
   full_name[ length_of_path ] = '/';
   memcpy( &full_name[length_of_path +1], file_name, num_chars );

   /* Pad name with blanks. */
   for( i = (length_of_path+1+num_chars); i < MAX_FULLNAME_CHARS; i++ )
      full_name[i] = ' ';

   return;

   
/* End of RPGCS_full_name() */
}
