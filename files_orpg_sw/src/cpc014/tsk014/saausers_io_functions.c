/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/02/18 16:42:04 $
 * $Id: saausers_io_functions.c,v 1.3 2004/02/18 16:42:04 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


/*******************************************************************************
Module:        saausers_io_functions.c

Description:   Set of modules to handle accessing the linear buffer SAAUSERSEL.DAT for 
	       cpc014/tsk014, saausers.  The primary function is to read in the clock 
	       hour accumulations generated by the Snow Accumulation Algorithm task, 
	       snowaccum.
	       
Input:         lbd, msg_id
	       int max_bufsize   maximum buffer size for outbuf
                  
Output:        none
   
Returns:       returns lbd
                  
Globals:       none 

Notes:         none

CCR#:          NA98-16301
               
Authors:       Dave Zittel, Meteorologist/Programmer, Radar Operations Center
               Version 1.0, August 2003
               
History:
               Initial implementation 8/08/03 - Zittel
               
*******************************************************************************/


#include "saausers_main.h"
int debugit = FALSE;

int write_SAAUSERSEL_lb( int msg_id ){

   int ret;
	
   /* write the message to the LB */
   if(debugit){
      fprintf(stderr,"SAAUSERSEL: msg_id = %d\n", msg_id);
      fprintf(stderr,"SAAUSERSEL: SAAUSER_BUFSIZ = %d, size of buffer = %d\n",
                SAAUSER_BUFSIZ, sizeof(usraccum)/MAX_HOURS);
   }

   ret = RPGC_data_access_write( SAAUSERSEL, &usraccum[msg_id-2], 
                                 SAAUSER_BUFSIZ, msg_id );

   if( ret == 2*MAX_SAA_RADIALS*MAX_SAA_BINS*sizeof(short) )
      return (1);

   else{
      if(debugit){fprintf(stderr,"LB_write failed. The return number = %d\n", ret);}
         return (-1);
   }

   return(0);
}

int write_USRSELHDR_lb( int msg_id ){

   int ret;

   /* write the message to the LB */
   if(debugit){
      fprintf(stderr,"USRSELHDR: msg_id = %d\n",msg_id);
      fprintf(stderr,"USRSELHDR: SAAUSER_BUFSIZ = %d, size of hskp = %d\n",
	                SAAUSER_BUFSIZ, sizeof(hskp_data));
   }

   ret = RPGC_data_access_write( SAAUSERSEL, &hskp_data, SAAUSER_BUFSIZ, msg_id );
   if (ret == SAAUSER_BUFSIZ )
      return (1);
   else{

      if(debugit){fprintf(stderr,"LB_write failed. The return number = %d\n", ret);}
      return (-1);

   }

   return(0);
}


int read_SAAUSERSEL_lb( int msg_id ){

   char *buffer; 
   int len;

   len = RPGC_data_access_read(SAAUSERSEL, &buffer, LB_ALLOC_BUF, msg_id);
   if( len > 0 ){

      if( len > SAAUSER_BUFSIZ ) 
         len = SAAUSER_BUFSIZ;

      memcpy( &usraccum[msg_id-2], buffer, len );
      free(buffer);

      if(debugit){fprintf(stderr,"Msg lng = %d, msg_id = %d\n",len,msg_id);}
      return (1);

   }
   else {

      if(debugit){fprintf(stderr,"LB_read failed.  The return number = %d\n", len);}
      return (-1);
   }

   return (0);
}
/*  SAA input/output functions   */

int read_USRSELHDR_lb( int msg_id ){

   char *buffer;
   int len;

   if(debugit){fprintf(stderr,"msg_id = %d\n", msg_id);}
   len = RPGC_data_access_read( SAAUSERSEL, &buffer, LB_ALLOC_BUF, msg_id );
   if( len > 0 ){

      if( len > SAAUSER_BUFSIZ )
         len = SAAUSER_BUFSIZ;

      memcpy(&hskp_data, buffer, len );
      free(buffer);
      if(debugit){fprintf(stderr,"Msg lng = %d, msg_id = %d\n",len,msg_id);}
         return (1);

   }
   else {

      if(debugit){fprintf(stderr,"LB_read failed.  The return number = %d\n", len);}
      return (-1);
   }

   return (0);
}
