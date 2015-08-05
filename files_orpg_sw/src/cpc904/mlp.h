/*   @(#)mlp.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * mlp.h of snet module
 *
 * SpiderX25
 * @(#)$Id: mlp.h,v 1.1 2000/02/25 17:14:34 john Exp $
 * 
 * SpiderX25 Release 8
 */



#define	MLP_NTIMS	4	/* Number of MLP timers used.		*/
#define	MLP_MT1		0	/* Used for frame loss detection	*/
#define	MLP_MT2		1	/* Used for buffer exhaustion recovery	*/
#define	MLP_MT3		2	/* Timer for reset confirm frames	*/
#define	MLP_DELAY	3	/* Timer for delay before reset		*/

/*
	Internal Data Structures.
*/
typedef struct mlp_message
{
	uint16			msg_mn_s ;	/* MN(S) of associated msg.  */
	uint8			msg_acked ;	/* If msg has been acked.    */
	uint8			msg_retent ;	/* Message on mlp_rethead    */
	struct mlp_slp *	msg_slp ;	/* SLP on which msg txed.    */
	mblk_t *		msg_dup ;	/* Duplicate message pointer */
	mblk_t *		msg_next ;	/* Next TX frame	     */
	mblk_t *		msg_retry ;	/* Other attempts at txing.  */
	mblk_t *		msg_retnext ;	/* Other messages with txerrs*/
} mlp_msg_t ;


/*
	Lower Interface structures.
*/
typedef struct mlp_slp
{
	uint8			slp_class ;	/* MLAPBDTE, MLAPBDCE	      */
	uint8			slp_dlpi_state;	/* DLPI state		      */
	uint32			slp_snid ;	/* Subnetwork identifier      */
	uint16			slp_l_index ;	/* Link index for I_LINK.     */
	uint16			slp_frame_cnt ; /* Max. no outstanding frames */
	uint16			slp_n_txed ;    /* Number to be acknowledged  */
	uint16			slp_N2 ;	/* LAPB N2 value	      */
	mblk_t *		slp_mlp_msgs ;	/* Messages being Txed on SLP */
	mblk_t *		slp_setsnidmsg;	/* Messages being Txed on SLP */
	queue_t *		slp_dn_q ;	/* LAPB write queue.	      */
	struct mlp_slp **	slp_stat_slp ;	/* This will be set to either *
						 * mlp_limbo, mlp_disc or     *
						 * mlp_conn.		      */
	struct mlp_slp *	slp_next ;	/* Next SLP in state list.    */
	struct mlp_slp *	slp_mlp_next ;	/* Next SLP in Global list.   *
						 * This list contains all     *
						 * SLP's which are associated *
						 * with the MLP.	      */
	struct mlp_subnetwork *	slp_mlp ;	/* Associated MLP structure   */
} mlp_slp_t ;


/*
	Upper Interface structures.
*/
typedef struct mlp_subnetwork
{
	uint8		mlp_state ;		/* State of MLP Stream.	      */
	uint8		mlp_txstate ;		/* State of tx side	      */
	uint8		mlp_rxstate ;		/* State of rx side	      */
	uint8		mlp_trace_active ;	/* set to 1 if tracing	      */
	uint32		mlp_snid ;		/* subnetwork identifier      */
	uint16		mlp_mv_s ;		/* Next MN(S) to transmit     */
	uint16		mlp_mv_t ;		/* Next txed MN(S) to ack     */
	uint16		mlp_mv_r ;		/* Next MN(S) expected on rx  */
	uint16		mlp_mw ;		/* MLP window size	      */
	uint16		mlp_mx ;		/* MLP guard region size      */
	uint16		mlp_mt1 ;		/* Frame lost timer	      */
	uint16		mlp_mt2 ;		/* Buffer exhaustion timer    */
	uint16		mlp_mt3 ;		/* Reset frame not confirmed  */
	uint16		mlp_mn1 ;		/* MLP intervention count     */
	uint16		mlp_rec ;		/* recovery timer	      */
	queue_t *	mlp_up_q ;		/* X25 lower read queue.      */
	queue_t *	mlp_top_q ;		/* MLP upper write queue.     */
	queue_t *	mlp_trace_queue ;	/* trace queue		      */
	queue_t *	mlp_ioctl_strm;		/* Stream Bind req rxed on    */
	mlpstats_t 	mlp_statstab ;		/* Statistics table	      */
	mlp_slp_t *	mlp_txing ;		/* SLP's to transmit over     */
	mlp_slp_t *	mlp_conn ;		/* List of all SLP's in a     *
						 * data transfer state.       */
	mlp_slp_t *	mlp_disc ;		/* List of all SLP's in a     *
						 * disconnected state.        */
	mlp_slp_t *	mlp_limbo ;		/* List of all SLP's in a     *
						 * unknown state.	      */
	mlp_slp_t *	mlp_slps ;		/* List of all SLP's.	      */
	mblk_t *	mlp_msgs ;		/* Insequence MLP frames.     */
	mblk_t *	mlp_reset_msgs ;	/* MLP frames received during *
						 * reset procedure.	      */
	mblk_t *	mlp_txhead ;		/* Head of TX messages.	      */
	mblk_t *	mlp_txtail ;		/* Tail of TX messages.	      */
	mblk_t *	mlp_rethead ;		/* Head of frames with txerrs */
	mblk_t *	mlp_rettail ;		/* Tail of frames with txerrs */
	mblk_t *	mlp_formatted ;		/* MLP frames with mlp_msg_t  */
	thead_t		thead ;
	s_timer_t	tmtab[MLP_NTIMS] ;
	uint32		mlp_upcmd;
	uint8		mlp_oktoconf ;		/* ok to confirm reset frame  */
	uint8		mlp_got_formatted ;	/* processing formatted frame *
						 * a data transfer state.     */
	struct lsapformat mlp_rem_addr;         /* Remote DTE address */
} mlp_t ;

/*
	Valid states for interface.
*/
#define	MLP_DISC	1	/* Stream is BOUND			    */
#define	MLP_BLOCKED	2	/* buffer failure			    */
#define	MLP_FLOW	3	/* Streams/protocol flow control	    */
#define	MLP_DATA	4	/* MLP in data transfer state		    */
#define	MLP_RESETDONE	5	/* Reset complete, connecting with PLP.	    */
#define	MLP_RESET_START	6	/* Reset process started but no reset sent  */
#define	MLP_RES_PEND	7	/* Reset frame has been sent		    */
#define	MLP_RESET	8	/* Reset frame has been received	    */
#define	MLP_RESETCONF	9	/* Reset confirm has been sent		    */


/*
    Used in M_SETOPTS. Mlp header = 2, LAPB extended = 3 hence 5.
*/
#define MLP_PROTO_OFFSET	5

#define	MLP_HEADER_SZ	2	/* Protocol header size	*/
#define	MLP_MS_MAX	4095	/* Max. sequence number	*/


#define	MLP_V_BIT	0x10	/* Void sequencing bit		*/
#define	MLP_S_BIT	0x20	/* Sequence check option bit	*/
#define	MLP_R_BIT	0x40	/* Reset request bit		*/
#define	MLP_C_BIT	0x80	/* Reset Confirm bit		*/

#define	DONTCARE	MLP_MS_MAX+1
#define	MLP_START_DELAY	40	/* Delay before reset txed	*/

#define	MLP_MNS_OK	1	/* Valid MN(S)				*/
#define	MLP_MNS_MISS	2	/* Valid MN(S), with dropped frame	*/
#define	MLP_MNS_INV	0	/* Invalid MN(S)			*/

#define	MSG		mlp_msg_t FAR *
#define	MLP_MSG(mp)	((MSG)mp->b_rptr)


/*
	MACROS used to setup/deassign MLP protocol header fields from
	the MLC.
*/

/*
	Retrieve the MN(S) from the MLC.
	ptr should be of type BUFP_T.
*/
#define MLP_GETMNS(ptr)	((*ptr & 0x0F) << 8) | (*(ptr+1));

/*
	Retrieve the control bits from the MLC.
	ptr should be of type BUFP_T.
*/
#define	MLP_GETBITS(ptr)	(uint8)(*ptr & 0xF0)

/*
	Setup the MLC.
	mn_s should be a uint16 and bits should be a uint8 with the control
	bits encoded in the lowest nibble via masks above.

	_SETMLCH sets up the high byte.
	_SETMLCL sets up the low byte.
*/
#define	MLP_SETUPMLCH(x, b)	((unchar)((unchar)b & 0xF0) |\
				(unchar)(((int)x & 0x0F00) >> 8))
#define	MLP_SETUPMLCL(x)	(x & 0xFF)

#define	SETUP_HEADER(mp) { \
				mp->b_wptr = mp->b_datap->db_lim ; \
				mp->b_rptr = (mp->b_wptr - MLP_HEADER_SZ) ; \
			 }

/*
	Macro's used for connid exchange between upper and lower interfaces.
*/
#define MLP_INDEX(lp)	(uint16)((int)(lp - mlp_dtab) + 1)
#define SLP_INDEX(slp)	(uint16)((int)(slp - mlp_slptab) + 1)



/*
	Conditions of state changes communicated to MLP by the SLPS.
*/
#define	SLP_FLOW		0	/* SLP has received a RNR	  */
#define	SLP_DATA		1	/* SLP flow control alleviated	  */
#define	SLP_ACKED		2	/* Remote SLP acked MLP frames	  */
#define	SLP_TXERR		3	/* SLP has transmissions errors	  */


/*
	These are proprietory primitives which cross the lower DLPI between
	LAPB and MLP.
*/
#define	MLP_L2PARAMS	DL_MAXPRIM+1	/* Returns SLP's N2 value */
#define	MLP_STATUS	MLP_L2PARAMS+1	/* Used for SLP ack to MLP */

/*
	Structures used to initialise MLP from LAPB.
*/
typedef struct mlp_l2_params
{
	uint32	mlp_prim;	/* MLP_L2PARAMS */
	uint8	N2;		/* LAPB N2 parameter */
	uint8	spare[3];
} mlp_l2par_t ;


/*
	Structures and states used for back acknowledgment from LAPB to MLP.
*/
typedef struct mlp_status
{
	uint32	mlp_prim;	/* MLP_STATUS */
	uint8	state;		/* State of specified frames */
	uint8	n_frames;	/* Number of frames */
	uint16	mn_s;		/* MN(s) of error/invalid frames */
} mlp_stat_t ;


/*
	Union used for finding primitive types.
*/
union MLP_primitives
{
	uint32		mlp_prim;
	mlp_l2par_t	mlp_l2pars;
	mlp_stat_t	mlp_status;
};

#define	MLPMONITOR(i) if ( lp->mlp_statstab.mlpmonarray[i]<(uint32)0xFFFFFFFF) \
			lp->mlp_statstab.mlpmonarray[i]++ ;

extern	uint32	mlpgmonarray[] ;

#define	MLPGMONITOR(i) if ( mlpgmonarray[i] < (uint32)0xFFFFFFFF ) \
			mlpgmonarray[i]++ ;

#define	MLPGMONITORBYTES(i, num) if ( mlpgmonarray[i] < (uint32)0xFFFFFFFF ) \
				   mlpgmonarray[i]+=num ;
