/*   @(#)ll_mon.h	1.1	07 Jul 1998	*/

/*******************************************************************************
                             Copyright (c) 1995 by                             

     ===     ===     ===         ===         ===                               
     ===     ===   =======     =======     =======                              
     ===     ===  ===   ===   ===   ===   ===   ===                             
     ===     === ===     === ===     === ===     ===                            
     ===     === ===     === ===     === ===     ===   ===            ===    
     ===     === ===         ===     === ===     ===  =====         ======    
     ===     === ===         ===     === ===     === ==  ===      =======    
     ===     === ===         ===     === ===     ===      ===    ===   =        
     ===     === ===         ===     === ===     ===       ===  ==             
     ===     === ===         ===     === ===     ===        =====               
     ===========================================================                
     ===     === ===         ===     === ===     ===        =====              
     ===     === ===         ===     === ===     ===       ==  ===             
     ===     === ===     === ===     === ===     ===      ==    ===            
     ===     === ===     === ===     === ===     ====   ===      ===           
      ===   ===   ===   ===   ===   ===  ===     =========        ===  ==     
       =======     =======     =======   ===     ========          ===== 
         ===         ===         ===     ===     ======             ===         
                                                                                
       U   c   o   n   X      C   o   r   p   o   r   a   t   i   o   n         
                                                                                
       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.
 
       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.
  
*******************************************************************************/

/* Link Level Monitor for UconX.25 Primitives and Definitions */

#ifndef _LL_MON_H
#define _LL_MON_H

#ifndef uchar
#define uchar unsigned char
#endif
#ifndef ushort
#define ushort unsigned short
#endif

#define N_MONITORED_SNIDS 16

/*
* Trace buffer entry
*/
typedef struct
{
   uchar         direction;
#define         INBOUND     0
#define         OUTBOUND    1
   uchar         address;
   uchar         control;
   uchar         control2;
   uchar         extended;
   union
   {
      uchar      frmr_octets [ 3 ];
      ushort     iframe_size;
   }frame_plus;
} CAPTURE_ENTRY;

/*
* Capture only attach structure
*/
 
typedef struct
{
   unsigned	 snid;
   queue_t       *wr_queue;
   queue_t       *rd_queue;
   queue_t       *Lrd_queue;
   CAPTURE_ENTRY capture_buf    [64] [16];
   int           valid_entries  [64];
   int           current;
   int           first;
   int           last;
   int           index;
   int           give_to_token;
   int           give_to_exp;
   int           up_token;
} LINK_MONITOR;


/* Control field */
 
#define SFORMAT  0x01    /* Supervisory format */
#define POLLBIT  0x10
#define FRMR     0x87

/* frame reject format */
 
struct  frmr_frame
{
   uchar         adr;
   uchar         ctl;
   uchar         rej_field;
   uchar         seq_field;
   uchar         wxyz_bits;
};


/*
* Values for ll_command are as follows.
*/
#define DL_MON_ATTACH    17
#define DL_MON_DETACH    18
#define DL_MON_DATA      19
/*
* The dl_monitor_link_layer structure conforms to the dlpi structures
* defined in dlpi.h. The value of dl_primitive for the x25llmon messages
* is DL_MONITOR_LINK_LAYER. 
*/
typedef struct {
    unsigned            dl_primitive;
    unsigned            dl_command;
    unsigned            dl_status;
    unsigned            dl_snid;
    unsigned            dl_entries;
    CAPTURE_ENTRY       dl_traces[16];
} dl_monitor_link_layer_t;
 



#endif /* _LL_MON_H */
