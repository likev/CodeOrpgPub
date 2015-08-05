/*
 * RCS info
 * $Author: port $
 * $Locker:  $
 * $Date: 1999/04/20 19:32:26 $
 * $Id: prcpadju_set_in_out.c,v 1.6 1999/04/20 19:32:26 port Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifdef SUNOS
#define prcpadju_set_in_out prcpadju_set_in_out_
#endif
#ifdef LINUX
#define prcpadju_set_in_out prcpadju_set_in_out__
#endif

/*******************************************************************    
   This function assigns the value of data to buffer_out. 
 
   Return:  There is no return value.  

********************************************************************/
void prcpadju_set_in_out( int *buffer_out, int *data ){

   buffer_out[0] = *data;

   return;
}
