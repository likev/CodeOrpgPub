/*
 * RCS info
 * $Author: port $
 * $Locker:  $
 * $Date: 1999/04/20 19:32:25 $
 * $Id: prcpadju_copy_input.c,v 1.5 1999/04/20 19:32:25 port Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifdef SUNOS
#define prcpadju_copy_input prcpadju_copy_input_
#endif
#ifdef LINUX
#define prcpadju_copy_input prcpadju_copy_input__
#endif

/*******************************************************************    
   This function copies the contents of buffer_in into buffer_out. 
 
   Return:  There is no return value.  

********************************************************************/
void prcpadju_copy_input( int *buffer_out, 
                          int *buffer_in, 
                          int *elements ){

   int i;

   for( i = 0; i < *elements; i++ ){
   
      buffer_out[i] = buffer_in[i];

   }

   return;
}
