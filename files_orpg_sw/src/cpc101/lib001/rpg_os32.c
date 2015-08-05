/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/10/04 19:44:15 $
 * $Id: rpg_os32.c,v 1.11 2005/10/04 19:44:15 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */ 
/********************************************************************

      Module:  os32.c

 Description:
	functions implementing the OS/32 intrinsic functions and
	system calls.

********************************************************************/
 

#include <rpg.h>
#include <ctype.h>
#include <string.h>

#define CHAR_BIT	8	/* number of bits in a char */

/********************************************************************
   Description:
      The functions receives a string of length "*size" and 
      initializes it to NULLs. 

   Inputs:
      size - pointer to int containing size of "message", in bytes.
      message - pointer to buffer to hold message.

   Outputs:
      status - pointer to int returning status of operation.
               -1 on failure, 0 on success.

   Returns:
      0 on failure, 1 on success.

   Notes:

*******************************************************************/
int RPG_clear_msg( int *size, char *message, int *status ){

   int i, len;

   len = *size;

   if( len < 0 || len > 256 ){

      *status = -1;
      return 0;

   }

   for( i = 0; i < len; i++ )
      *(message + i) = 0;

   *status = 0;
   return 1;

}

/*********************************************************************
   Description:
      Writes a message to task log file.

   Inputs:
      message - pointer to string.

   Outputs:

   Returns:
      Always returns 0.

   Notes:
      It is assumed that "message" is terminated with '$'.

**********************************************************************/
int RPG_send_msg( char *message ){

    int pos = 0;

    /* Make sure message is null terminated */
    while( pos < 256 )
    {
      if( message[ pos ] == '$' )
      {
        message[ pos ] = '\0';
        break;
      }
      else if( message[ pos ] == '\0' )
      {
        break;
      }
      else
      {
        pos++;
      }
    }

   if( pos >= 256 )
      LE_send_msg( GL_INFO, "STRING MUST BE TERMINATED WITH $\n" );
   else
      LE_send_msg( GL_INFO, "%s\n", message );

   return 0;

}

/********************************************************************

   Description:

	This function loads the right byte of i with the kth byte of 
	argument j. k = 0 for the left byte of first halfword; k = 1 
	for the right byte of first halfword; k = 2 for the left byte 
	of second halfword; and so on.

    Return:	The return value is meaningless.

*********************************************************************/
int ilbyte (int *i, short j[3000], int *k){
    int shift;

    if ((*k % 2) == 0)
	shift = 8;
    else 
	shift = 0;

    *i = (*i & 0xffffff00) | (0xff & (j [*k >> 1] >> shift));
    return (0);
}

/********************************************************************

	This function stores the right byte of i into the kth byte of 
	argument j. k = 0 for the left byte of first halfword; k = 1 
	for the right byte of first halfword; k = 2 for the left byte 
	of second halfword; and so on.

    Return:	The return value is meaningless.

*/

int isbyte (int *i, short *j, int *k)
{
    int ind;

    ind = *k >> 1;
    if ((*k % 2) == 0) 
        j [ind] = ((*i & 0xff) << 8) | (j [ind] & 0xff);
    else 
        j [ind] = (*i & 0xff) | (j [ind] & 0xff00);

    return (0);
}



/**************************************************************************
 Description: This function implements the OS32 BTEST function for
              INTEGER*4 data.  Note that the OS32 BTEST function is not
              limited to testing bits within a four-byte word (i.e., the bit
              offset can be quite large).
       Input: pointer to INTEGER*4 data
              pointer to bit offset
              pointer to storage for return value
      Output: the return value is set to reflect whether or not the
              indicated bit is set
     Returns:  0
       Notes: This function is not called directly from Fortran (OS32BTEST
              is the Fortran interface "shell").

              Instead of returning a logical value, a new variable is added
              to return the result. This will increase portability. 

              This function accomodates Big Endian and Little Endian
              architectures.  The legacy software was written for a Big
              Endian architecture, so the bit offset will be a Big Endian
              bit offset.  We simply "reshuffle" the calculated byte offset
              for the Little Endian architecture:

                 Big Endian:  ByteN+3 ByteN+2 ByteN+1 ByteN
              Little Endian:  ByteN   ByteN+1 ByteN+2 ByteN+3 

 **************************************************************************/
int
btest (unsigned char *data, int *off, int *result)
{
    int byte_offset;
    int bit_offset;

    byte_offset = *off / CHAR_BIT ;

#ifdef LITTLE_ENDIAN_MACHINE
    if ((byte_offset % 4) == 0) {
        byte_offset = byte_offset + 3 ;
    }
    else if ((byte_offset % 4) == 1) {
        byte_offset = byte_offset + 1 ;
    }
    else if ((byte_offset % 4) == 2) {
        byte_offset = byte_offset - 1 ;
    }
    else {
        byte_offset = byte_offset - 3 ;
    }
#endif

    /*
     * to match Concurrent bitmaps ... leftmost bit is 0 bit
     * e.g., offset of 3 bits from the left translates to a left bit
     * shift of four ...
     */
    bit_offset = (CHAR_BIT - 1) - (*off % CHAR_BIT) ;

    if (data [byte_offset] & (1 << bit_offset)) {
	*result = 1;
    }
    else {
	*result = 0;
    }

    return (0);

/*END of btest()*/
}



/**************************************************************************
 Description: This function implements the OS32 BTEST function for
              INTEGER*2 data.  Note that the OS32 BTEST function is not
              limited to testing bits within a two-byte word (i.e., the bit
              offset can be quite large).
       Input: pointer to INTEGER*2 data
              pointer to bit offset
              pointer to storage for return value
      Output: the return value is set to reflect whether or not the
              indicated bit is set
     Returns:  0
       Notes: This function is not called directly from Fortran (OS32SBTEST
              is the Fortran interface "shell").

              Instead of returning a logical value, a new variable is added
              to return the result. This will increase portability. 

              This function accomodates Big Endian and Little Endian
              architectures.  The legacy software was written for a Big
              Endian architecture, so the bit offset will be a Big Endian
              bit offset.  We simply "reshuffle" the calculated byte offset
              for the Little Endian architecture:

                 Big Endian:  ByteN+1 ByteN
              Little Endian:  ByteN   ByteN+1

 **************************************************************************/
int
btest_short (unsigned char *data, int *off, int *result)
{
    int byte_offset;
    int bit_offset;
 

    byte_offset = *off / CHAR_BIT ;

#ifdef LITTLE_ENDIAN_MACHINE
    if (byte_offset % 2) {
        byte_offset = byte_offset - 1 ;
    }
    else {
        byte_offset = byte_offset + 1 ;
    }
#endif

    /*
     * to match Concurrent bitmaps ... leftmost bit is 0 bit
     * e.g., offset of 3 bits from the left translates to a left bit
     * shift of four ...
     */
    bit_offset = (CHAR_BIT - 1) - (*off % CHAR_BIT) ;

    if (data [byte_offset] & (1 << bit_offset)) {
	*result = 1;
    }
    else {
	*result = 0;
    }

    return (0);

/*END of btest_short()*/
}



/**************************************************************************
 Description: This function implements the OS32 BCLR function for
              INTEGER*4 data.  Note that the OS32 BCLR function is not
              limited to clearing bits within a four-byte word (i.e., the
              bit offset can be quite large).
       Input: pointer to INTEGER*4 data
              pointer to bit offset
      Output: the specified bit is cleared
     Returns:  0
       Notes: This function accomodates Big Endian and Little Endian
              architectures.  The legacy software was written for a Big
              Endian architecture, so the bit offset will be a Big Endian
              bit offset.  We simply "reshuffle" the calculated byte offset
              for the Little Endian architecture:

                 Big Endian:  ByteN+3 ByteN+2 ByteN+1 ByteN
              Little Endian:  ByteN   ByteN+1 ByteN+2 ByteN+3 

 **************************************************************************/
int
os32bclr (unsigned char *data, int *off)
{
    int byte_offset;
    int bit_offset;
 
    byte_offset = *off / CHAR_BIT ;

#ifdef LITTLE_ENDIAN_MACHINE
    if ((byte_offset % 4) == 0) {
        byte_offset = byte_offset + 3 ;
    }
    else if ((byte_offset % 4) == 1) {
        byte_offset = byte_offset + 1 ;
    }
    else if ((byte_offset % 4) == 2) {
        byte_offset = byte_offset - 1 ;
    }
    else {
        byte_offset = byte_offset - 3 ;
    }
#endif

    /*
     * to match Concurrent bitmaps ... leftmost bit is 0 bit
     * e.g., offset of 3 bits from the left translates to a left bit
     * shift of four ...
     */
    bit_offset = (CHAR_BIT - 1) - (*off % CHAR_BIT) ;

    data [byte_offset] &= ~(1 << bit_offset);

    return (0);

/*END of os32bclr()*/
}



/**************************************************************************
 Description: This function implements the OS32 BCLR function for
              INTEGER*2 data.  Note that the OS32 BCLR function is not
              limited to clearing bits within a two-byte word (i.e., the bit
              offset can be quite large).
       Input: pointer to INTEGER*2 data
              pointer to bit offset
      Output: the specified bit is cleared
     Returns:  0
       Notes: This function accomodates Big Endian and Little Endian
              architectures.  The legacy software was written for a Big
              Endian architecture, so the bit offset will be a Big Endian
              bit offset.  We simply "reshuffle" the calculated byte offset
              for the Little Endian architecture:

                 Big Endian:  ByteN+1 ByteN
              Little Endian:  ByteN   ByteN+1

 **************************************************************************/
int
os32sbclr(unsigned char *data, int *off)
{
    int byte_offset;
    int bit_offset;
 
    byte_offset = *off / CHAR_BIT ;

#ifdef LITTLE_ENDIAN_MACHINE
    if (byte_offset % 2) {
        byte_offset = byte_offset - 1 ;
    }
    else {
        byte_offset = byte_offset + 1 ;
    }
#endif

    /*
     * to match Concurrent bitmaps ... leftmost bit is 0 bit
     * e.g., offset of 3 bits from the left translates to a left bit
     * shift of four ...
     */
    bit_offset = (CHAR_BIT - 1) - (*off % CHAR_BIT) ;

    data [byte_offset] &= ~(1 << bit_offset);

    return (0);

/*END of os32sbclr()*/
}



/**************************************************************************
 Description: This function implements the OS32 BSET function for
              INTEGER*4 data.  Note that the OS32 BSET function is not
              limited to setting bits within a two-byte word (i.e., the bit
              offset can be quite large).
       Input: pointer to INTEGER*4 data
              pointer to bit offset
      Output: the specified bit is set
     Returns:  0
       Notes: This function accomodates Big Endian and Little Endian
              architectures.  The legacy software was written for a Big
              Endian architecture, so the bit offset will be a Big Endian
              bit offset.  We simply "reshuffle" the calculated byte offset
              for the Little Endian architecture:

                 Big Endian:  ByteN+3 ByteN+2 ByteN+1 ByteN
              Little Endian:  ByteN   ByteN+1 ByteN+2 ByteN+3 

 **************************************************************************/
int
os32bset(unsigned char *data, int *off)
{
    int byte_offset;
    int bit_offset;
 
    byte_offset = *off / CHAR_BIT ;

#ifdef LITTLE_ENDIAN_MACHINE
    if ((byte_offset % 4) == 0) {
        byte_offset = byte_offset + 3 ;
    }
    else if ((byte_offset % 4) == 1) {
        byte_offset = byte_offset + 1 ;
    }
    else if ((byte_offset % 4) == 2) {
        byte_offset = byte_offset - 1 ;
    }
    else {
        byte_offset = byte_offset - 3 ;
    }
#endif

    /*
     * to match Concurrent bitmaps ... leftmost bit is 0 bit
     * e.g., offset of 3 bits from the left translates to a left bit
     * shift of four ...
     */
    bit_offset = (CHAR_BIT - 1) - (*off % CHAR_BIT) ;

    data [byte_offset] |= (1 << bit_offset);

    return (0);

/*END of os32bset()*/
}



/**************************************************************************
 Description: This function implements the OS32 BSET function for
              INTEGER*2 data.  Note that the OS32 BSET function is not
              limited to setting bits within a two-byte word (i.e., the bit
              offset can be quite large).
       Input: pointer to INTEGER*2 data
              pointer to bit offset
      Output: the specified bit is set
     Returns:  0
       Notes: This function accomodates Big Endian and Little Endian
              architectures.  The legacy software was written for a Big
              Endian architecture, so the bit offset will be a Big Endian
              bit offset.  We simply "reshuffle" the calculated byte offset
              for the Little Endian architecture:

                 Big Endian:  ByteN+1 ByteN
              Little Endian:  ByteN   ByteN+1

 **************************************************************************/
int
os32sbset(unsigned char *data, int *off)
{
    int byte_offset;
    int bit_offset;
 
    byte_offset = *off / CHAR_BIT ;

#ifdef LITTLE_ENDIAN_MACHINE
    if (byte_offset % 2) {
        byte_offset = byte_offset - 1 ;
    }
    else {
        byte_offset = byte_offset + 1 ;
    }
#endif

    /*
     * to match Concurrent bitmaps ... leftmost bit is 0 bit
     * e.g., offset of 3 bits from the left translates to a left bit
     * shift of four ...
     */
    bit_offset = (CHAR_BIT - 1) - (*off % CHAR_BIT) ;

    data [byte_offset] |= (1 << bit_offset);

    return (0);

/*END of os32sbset()*/
}

/*******************************************************************

	This is the os32 DATE function.

    Return:	The return value is meaningless.

*******************************************************************/

int date (int *yr)
{
    time_t tm;
    int y, mon, d, h, m, s;

    tm = time (NULL);
    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
    if( y >= 2000 )
       yr[0] = y - 2000;
    else
       yr [0] = y - 1900;
    yr [1] = mon;
    yr [2] = d;

    return (0);
}


/*******************************************************************

	This is the os32 ICLOCK function.

*******************************************************************/

int iclock (int *sw, int *buf)
{
    time_t tm;
    int y, mon, d, h, m, s;

    tm = time (NULL);
    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
    if (*sw == 0) {
	buf [0] = h;
	buf [1] = m;
	buf [2] = s;
    }
    else {
	*buf = h * 3600 + m * 60 + s;
    }

    return (0);
}

#define MIN_DATE	70

/*******************************************************************

	This is the os32 T41194__GETTIME function.

    Return:	The return value is meaningless.

*******************************************************************/

int t41194__gettime (double *comp, double *ref, int *ms, short *date)
{
    double dd;
    int ii;
    time_t tm;
    int y, mon, d, h, m, s;

    dd = *comp;
    ii = (int)dd;
    d = ii % 100;
    ii = ii / 100;
    mon = ii % 100;
    ii = ii / 100;
    y = (ii % 100);

    if( y < MIN_DATE )
       y = y + 2000;
    else
       y = y + 1900;

    dd = (dd - (double)((int)dd)) * 1000000.;
    ii = (int)dd;
    s = ii % 100;
    ii = ii / 100;
    m = ii % 100;
    ii = ii / 100;
    h = ii % 100;

    tm = 0;
    unix_time (&tm, &y, &mon, &d, &h, &m, &s);
    *ms =  (h * 3600 + m * 60 + s) * 1000;   
    *date = (tm / 86400) + 1;

    return (0);
}

/********************************************************************

    Description: This is the emulated OS32 WAIT function, which is a
		sleep function.

    Inputs:	delay: specifying the amount of sleep time.

		unit: The unit of the value "delay"; 1 for milliseconds;
			2 for seconds; 3 for minutes.

    Output:	status:	returning an error status: 1 means no error;
			3 means invalid "unit" specification.
			4 means invalid "delay" specification.

    Return:	The return value is meaningless.

    Notes:	Be careful that UNIX has a wait system function.
		This function may return without finishing the requested
		sleeping when there is an interupt.

********************************************************************/

int wait_c (int *delay, int *unit, int *status)
{
    int t;

    switch (*unit) {

	case 1:
	    t = *delay;
	    break;
	case 2:
	    t = *delay * 1000;
	    break;
	case 3:
	    t = *delay * 60000;
	    break;

	default:
	    *status = 3;
	    return (0);
    }

    if (t <= 0) {
	*status = 4;
	return (0);
    }

    msleep (t);

    *status = 1;
    return (0);
}


/********************************************************************

    Description: This is the emulated OS32 SNDMSG function. 

    Inputs:	rcv_name: 8-byte receiver's name. If all first 4 
			bytes are zero, the receiver is the task 
			itself.

		msg: 64 byte message. The first 8 bytes are the 
			sender's name.

    Output:	status:	returning an error status: 0 means no error;
			> 1 means an error (See pp. 4-41, OS32
			system support run-time library (RTL) 
			reference manual)

    Return:	The return value is meaningless.

    Notes:	For the moment, we only print out the message.

********************************************************************/
void
sndmsg (char *rcv_name, int *message, int *status)
{
    char buf [65], name [9];
    char *msg;
    int num_chars = 64;
    int num_blanks = 0;

    if ((rcv_name == NULL) || (message == NULL) || (status == NULL)) {
        return ;
    }

    (void) strncpy (name, rcv_name, 8);
    name [8] = 0;

    msg = (char *) message;

    /*
     * Remove any leading non-alphanumeric characters.
     */
    while(!isalnum( (int) msg[num_blanks])){
       num_blanks++; 
       num_chars--;
    }

    (void) strncpy (buf, &msg[num_blanks], num_chars);
    buf [num_chars] = 0;

    /* For legacy support.  Status log messages are sent to "STATMON" */
    if( strncmp( name, "STATMON", (size_t) 7 ) == 0 ){

       /* Logged messages always have 4 prefixed blanks to message. */
       if( strncmp( msg, "    ", (size_t) 4 ) == 0 ){

          LE_send_msg (GL_STATUS | LE_RPG_GEN_STATUS, "%s\n", buf);
          *status = 0;

          return;

       }

    }  
 
    /* If falls through to here, log message to task's log file. */
    LE_send_msg (GL_INFO, "%s\n", buf);

    *status = 0;

/*END of sndmsg()*/
}


/********************************************************************

    Description: This is the emulated OS32 QUEUE function. 

    Inputs:     rcv_name: 8-byte receiver's name. If all first 4 
                        bytes are zero, the receiver is the task 
                        itself.

                parm: 4 byte right-justified item to be sent to 
                        receiver.

    Output:     status: returning an error status: 0 means no error;
                        > 1 means an error (See pp. 4-38, OS32
                        system support run-time library (RTL) 
                        reference manual)

    Return:     The return value is meaningless.

    Notes:      For this moment, we only print out the parameter.

********************************************************************/
int
queue (char *rcv_name, int *parm, int *status)
{
    char name [9];

    if ((rcv_name == NULL) || (parm == NULL) || (status == NULL)) {
        return (0) ;
    }

    (void) strncpy (name, rcv_name, 8);
    name [8] = 0;

    LE_send_msg (GL_INFO,
                 "OS32 queue param sent to %s: %d\n",
                 name, *parm);

    *status = 0;

    return (0);

/*END of queue()*/
}

/*********************************************************************

    Description: The following is an implementation of the OS32 list
		processing routinue deflst, list initialization.

    Inputs:	list - The list array;
		size - list size;

    Output:	list - Initialized list array;

    Return:	The return value is meaningless. On failure this 
		function calls PS_task_abort to terminate the task.

    Notes: 	For a description of the OS32 list, refer to p13-20,
		"FORTRAN VII REFERENCE (48-017 F00 R03)". The first 
		two elements in the list array are used for list 
		control (header). list[0] is the size of the list 
		(must be <= 32767). 
		list[1] = tpt + (nitems << 16), where tpt is the 
		pointer pointing to the location of the "next" new top 
		element and nitems is the number of elements currently
		in the list.

*********************************************************************/

#define LIST_HEADER 	2	/* list header size in number of ints */

int deflst (int *list, int *size)
{

    if (*size < 1 || *size > 32767)
	PS_task_abort ("deflst error: size (%d) incorrect\n", *size);
    else
	list [0] = *size;
    list [1] = 0;

    return (0);
}

/*********************************************************************

    Description: The following is an implementation of the OS32 list
		processing routinue atl, adding a top element.

    Inputs:	value - value to add to the top of the list;
		list - The list array;

    Output:	status - return status;
		list - midified list array;

    Return:	The return value is meaningless.

*********************************************************************/

int atl (int *value, int *list, int *status)
{
    int tpt, nitems, size;

    tpt = list [1] & 0xffff;
    nitems = (list [1] >> 16) & 0xffff;
    size = list [0];

    if (nitems >= size) {
	*status = 1;
	return (0);
    }

    list [tpt + LIST_HEADER] = *value;
    tpt = (tpt + 1) % size;
    nitems++;

    list [1] = tpt + (nitems << 16);

    *status = 0;
    return (0);
}


/*********************************************************************

    Description: The following is an implementation of the OS32 list
		processing routinue abl, adding a bottom element.

    Inputs:	value - value to add to the bottom of the list;
		list - The list array;

    Output:	status - return status;
		list - midified list array;

    Return:	The return value is meaningless.

*********************************************************************/

int abl (int *value, int *list, int *status)
{
    int tpt, nitems, size;

    tpt = list [1] & 0xffff;
    nitems = (list [1] >> 16) & 0xffff;
    size = list [0];

    if (nitems >= size) {
	*status = 1;
	return (0);
    }

    nitems++;
    list [((tpt - nitems + size) % size) + LIST_HEADER] = *value;

    list [1] = tpt + (nitems << 16);

    *status = 0;
    return (0);
}


/*********************************************************************

    Description: The following is an implementation of the OS32 list
		processing routinue rtl, removing a top element.

    Inputs:	list - The list array;

    Output:	value - value removed from the top of the list;
		status - return status;
		list - midified list array;

    Return:	The return value is meaningless.

*********************************************************************/

int rtl (int *value, int *list, int *status)
{
    int tpt, nitems, size;

    tpt = list [1] & 0xffff;
    nitems = (list [1] >> 16) & 0xffff;
    size = list [0];

    if (nitems == 0) {
	*status = 2;
	return (0);
    }

    tpt = (tpt - 1 + size) % size;
    *value = list [tpt + LIST_HEADER];
    nitems--;

    list [1] = tpt + (nitems << 16);

    if (nitems > 0)
	*status = 1;
    else
	*status = 0;

    return (0);
}

/*********************************************************************

    Description: The following is an implementation of the OS32 list
		processing routinue rbl, removing a bottom element.

    Inputs:	list - The list array;

    Output:	value - value removed from the bottom of the list;
		status - return status;
		list - midified list array;

    Return:	The return value is meaningless.

*********************************************************************/

int rbl (int *value, int *list, int *status)
{
    int tpt, nitems, size;

    tpt = list [1] & 0xffff;
    nitems = (list [1] >> 16) & 0xffff;
    size = list [0];

    if (nitems == 0) {
	*status = 2;
	return (0);
    }

    *value = list [((tpt - nitems + size) % size) + LIST_HEADER];
    nitems--;

    list [1] = tpt + (nitems << 16);

    if (nitems > 0)
	*status = 1;
    else
	*status = 0;

    return (0);
}

/*********************************************************************

    Description: The following is an implementation of the OS32 list
		processing routinue lstfun.

    Inputs:	fun - function switch;
		list - The list array;
		value - value to add to the list;

    Output:	value - value removed from the list;
		status - return status;
		list - midified list array;

    Return:	The return value is meaningless.

*********************************************************************/

int lstfun (int *fun, int *value, int *list, int *status)
{

    switch (*fun) {

	case 1:
	    return (atl (value, list, status));
	case 2:
	    return (rtl (value, list, status));
	case 3:
	    return (abl (value, list, status));
	case 4:
	    return (rbl (value, list, status));
	default:
	    *status = 3;
	    return (0);
    }
}

/*******************************************************************    
   This function takes an integer value, *value, and converts to 
   a left-justified 12 character string.  num_chars is the number
   of characters in the converted string.
 
   Return:  The return value is meaningless

********************************************************************/
int itoc_os32( int *value, int *num_chars, char string[12] ){

   int i;

   /* Initialize the string elements to '\0' */
   for( i = 0; i < 12; i++ ){
      string[i] = '\0';
   }

   /* Convert integer value to left justified character string */
   sprintf( string, "%-d", *value );

   /* Store the number of characters in string for return */
   *num_chars = strlen( string ); 

   return ( 0 ) ;

}


/**************************************************************************
 Description: This function replaces the OS32 IOERR() routine.
       Input: pointer to integer*4 array of at least five elements
              containing the SVC1 parameter block used to initiate the
              I/O through an earlier SYSIO call
      Output: value pointed at by pblk is placed in status 
     Returns: none
     Globals: none
       Notes: ORPG uses replacement SYSIO call that places either 0 or 1 in
              the first element of pblk
 **************************************************************************/
void ioerr(fint *pblk, fint *status)
{

   *status = *pblk & 0xffff;


/*END of ioerr()*/
}


/**************************************************************************
 Description: ORPG replacement for the legacy S4CMAD routine.
       Input:
      Output:
     Returns: none (ignored)
     Globals: none
       Notes:
 **************************************************************************/
fint s4cmad(fint *variable,
            fint *address)
{

   *address = (fint) variable ;

   return(0) ;

/*END of s4cmad()*/
}

#define A3CM04E__FULL_NAME a3cm04e__full_name

#ifdef LINUX
#define a3cm04e__full_name a3cm04e__full_name__
#endif

#ifdef SUNOS
#define a3cm04e__full_name a3cm04e__full_name_
#endif

#define MAX_FILENAME_CHARS              12
#define MAX_FULLNAME_CHARS             255

/******************************************************************

   Description: 
      Prefixes "temporary directory" path to file name.  The 
      temporary directory path is returned from MISC_get_work_dir.

   Inputs:
      file_name - FORTRAN character array (up to 12 characters)

   Outputs:
      full_name - FORTRAN character array (up to 255 characters).
                  Gives the fully quanified path for "file_name".

   Returns:
      Always returns 0.

   Notes:

******************************************************************/
int a3cm04e__full_name( char *file_name, char *full_name ){

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
   
      return 0;

   }
   
   /* Prepend the "work_dir" to "file_name".   "work_dir" does not
      include the trailing '/' */
   memcpy( full_name, work_dir, length_of_path );
   full_name[ length_of_path ] = '/';
   memcpy( &full_name[length_of_path +1], file_name, num_chars );

   /* Pad name with blanks. */
   for( i = (length_of_path+1+num_chars); i < MAX_FULLNAME_CHARS; i++ )
      full_name[i] = ' ';

   return 0;

   
/* End of a3cm04e__full_name() */
}
