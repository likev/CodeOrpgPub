/*   @(#) abmstructs.h 99/12/23 Version 1.10   */

/*******************************************************************************
                             Copyright (c) 1996 by                             

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

/*
Modification history:
 
Chg Date       Init Description
1.  13-AUG-96  LMM  Removed def of MAX_LINKS (now defined in include/targets)
2.  30-aug-96  pmt  Get rid of union; caused problems when compiling on
		    different client platforms (used by monitor only)
3.  24-feb-96  pmt  removed unused x_c_frame struct. fixed frmr struct. 
4.  11-nov-98  rjp  Added support for end-to-end ack.
5.   3-dec-98  rjp  Fixed support for end-to-end ack.
*/

/* supervisory or unnumbered frame */

struct	c_frame 
{
   bit8		adr;
   bit8		ctl;
   bit8		ctl2;
};

/* throw in the however many bytes of adr, then use this */
struct  frmr_buff
{
   bit8         ctl;            /* deleted 2nd ctl byte; this is unnum. frame -- pmt */
   bit8         frmr_ctl;
   bit8         frmr_nrns;
   bit8         frmr_flags;
};
struct  frmr_buff2
{
   bit8         ctl;            /* deleted 2nd ctl byte; this is unnum. frame -- pmt */
   bit8         frmr_ctl;
   bit8         frmr_ctl2;
   bit8         frmr_ns;
   bit8         frmr_nr;
   bit8         frmr_flags;
};



/* DTE PRIMARY STATE STRUCTURE 
 *
 * every thing needed in the DTE primary
 * so that a reset can be done
 */

struct pri_state 
{
   bit8		vs;  			/* send variable */
   bit8		npr; 			/* received frame indicator */
   bit8		x;   			/* mark last frame before timeout */

   bit8		substate; 		/* substate in the primary */
   bit16	retries;  		/* count number of retries */
   mblk_t	*slot	   [ MOD128_MASK + 1 ];
   mblk_t	*dupslot   [ MOD128_MASK + 1 ];	/* shadow slots for retransmission */
   
/* array of slots to send frame with. When the slot is in the state S2
 * this is a pointer to the frame to transmit
*/

   bit8		slot_state [ MOD128_MASK + 1 ];

/* array of slot states : states can be S1, S2, S3 or S4 */
	
   mblk_t	*frmr_memory; 		/* remember last frmr tranmsitted */

/* data space for building frmr information */

   bit8		pr_ctl; 		/* value in the control field */
   bit8		pr_ctl2; 		/* value in the control field */
   bit8		pr_cause; 		/* cause */
   bit8		resp_ind; 		/* set bit to show reject cmd or resp */
};

typedef struct
{               /* Frame counts */

/* From HDLC driver */

   bit32        Xgood;
   bit32        Rgood;
   bit32        Wunder;
   bit32        Rover;
   bit32        Rtoo;
   bit32        Rcrc;
   bit32	Rabt;

/* From lapb */

   bit32        IFrames;                     /* I-Frames */
   bit32        RRs;                         /* S-Frames */
   bit32        RNRs;
   bit32        REJs;
   bit32        XSFrames;
   bit32        DISCs;                       /* U-Frames */
   bit32        SABMs;
   bit32        SABMEs;
   bit32        SARMs;
   bit32        UAs;
   bit32        FRMRs;
   bit32        XUFrames;
   bit32        PFA;                         /* Poll/Final bits, Address A */
   bit32        PFB;                         /* Address B */
   bit32        PFX;                         /* Bad Address */
   bit32        WNDW;                        /* Xmit window full */
} FrameStat;



typedef		struct
{
   int		in_use;
   queue_t	*wr_queue;
   queue_t	*rd_queue;
   bit32	link;
   int		No_Additions_Above;	/* dummy to mark where bzero starts*/
   int		Wretval_to;		/* return value from timeout call  */
   int		Rretval_to;		/* return value from timeout call  */
   bit32        dl_addr_type;           /* DTE (0) or DCE (1) addressing   */
   bit32        dl_service;		/* data link, monitor, SP loopback */
/* Max frame size INCLUDING 2-byte hdlc header                             */
   bit32        dl_max_frame;
   bit32        dl_baud_rate;           /* line speed                      */
/* Timer T1, at the end of which retransmission of a frame is initiated    */
   bit32        dl_t1;
/* Timer T3, is the timeout interval for completion of link initialization */
   bit32        dl_t3;
/* N2, the maximum number of trans/retransmissions of a given frame        */
   bit32        dl_n2;
/* K, the maximum number of information frames outstanding at any time     */
   bit32        dl_k;
   bit32	dl_rr_delay;
   bit32	dl_Reuters;
   bit32	dl_modem_sigs;
   int		Reuters_token;
   int		rr_delay_token;
   bit32	dl_rualive;
   int		rualive_token;
   int		rualive_retries;
   int		rualive_expired;
   bit32	dl_unack_max;
   bit32	unack_cur;
   int		frtim;			/* Return value from timeout call  */
   int		fr3tim;			/* Return value from timeout call  */
   int		dce_ret_val;
   int		dce_expired;

   int		primadr_len;	/* number of bytes in address */
   int		secadr_len;
   bit8		primadr[MAX_ADDR_LEN];	
   bit8		secadr[MAX_ADDR_LEN];

   int		ctl_len;

   dl_options	dl_options_list;	/* optional adccp functions */	
   int 		mod;
   int		s_c_frame;		/* sizeof c_frame */
   int		sabme;		/* flag used to indicate a sabme rec'd or sent */
   int		sent_sim;
   int		sim_set;

/* secondary variables */

   int		uc_sec_state;
   int		sec_event;
   int		l_vr;			/* #5				*/
   int		poll_request;
   bit8		sec_response;
   bit8		frmr_ctl;
   bit8		frmr_ctl2;
   bit8		frmr_cause;
   bit8		frmr_resp_ind;
   int		frmr_start_t1;
   int		RNR_TO_retval;
   int		RNRexpired;
   int		RNRstate;
   mblk_t       *have_mproto;
   mblk_t	*srej_buf[ 128 ];
   int		srej_index;
   int		srej_found;

/* primary variables */

   int		link_state;
   int		pri_event;
   char         sec_state	[ 6 ];
   struct  	pri_state	pri_state;
   bit8    	un_ack;
   int		err_prim; 
   int		dlpi_err;
   int		T1expired;
   int		T3expired;
   int		reset_req;
   int		ack_recd;		/* #4 count of msgs acked */
   int		l_mvr;			/* #5 copy of vr 		*/
   int		l_hvr;			/* #5 copy of vr 		*/
   int		l_rvs;			/* #5 track received s variable */
   int		l_rcyvr;		/* #5 recovery vr (end to end)	*/
   int		l_rxold;		/* #5 recovering (end to end)	*/
   int		l_duplicate;		/* #4 Note duplicate */
   int		poll_state; 
   FrameStat    TxFrames;
   FrameStat    RxFrames;

   mblk_t	*save_msg       [ 4 ];
   int		 Fill_token;

} dlsap2stream;

/* Capture buffer entry */

typedef         struct
{
   bit8		direction;
#define		INM	0
#define		OUTM	1
   bit8		addr_len;
   bit8		address[8];
   bit8		mod128;
   bit8		control;
   bit8		ctl2;
   bit8		frmr_octets [ 3 ];
   bit16	iframe_size;
} capture_entry;

/* Capture only bind structure */

typedef         struct
{
   int           in_use;
   queue_t       *wr_queue;
   queue_t       *rd_queue;
   bit32         link;
   dlsap2stream	 *ctrl; 
   capture_entry capture_buf	[ 64 ] [ 16 ];
   int		 valid_entries  [ 64 ];
   int	         current;
   int           first;
   int           last;
   int		 index;
   int		 give_to_token;
   int           give_to_exp;
   int		 up_token;
} capture_bind; 


/* header structure for lapb messages: 3 fields of the HDLC_HDR +
   lapb internal fields. */

typedef		struct
{
   int		command;
   int		status;
   int		count;
   int		Tag_Buffer;
} HDLC_HDR_X;


typedef		struct
{
   bit8		ll_type;
} ll_standard;
