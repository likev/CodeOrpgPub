/*   @(#) xdebug.h 99/12/23 Version 1.1   */
/*****************************************************************************

    @@@@      @@@@@             Copyright 1991 by          @@@@@       @@@@@
    @@@@      @@@@@                                        @@@@@       @@@@@
    @@@@      @@@@@                                          @@@@     @@@@
    @@@@      @@@@@    @@@@@@@@       @@ @@      @@@@     @@@@@@@     @@@@
    @@@@      @@@@@   @@@@@@@@@@   @@@@@ @@@@@   @@@@     @@@@@@@@@ @@@@@
    @@@@      @@@@@ @@@@@@@@@@@@  @@@@@@ @@@@@@  @@@@@@   @@@@@@@@@ @@@@@
    @@@@      @@@@@@@@@@@    @@@  @@@@@@ @@@@@@  @@@@@@   @@@@  @@@@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@@@@  @@@@  @@@@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@@@@  @@@@   @@@@@
    @@@@      @@@@@@@@@         @@@@@@     @@@@@@@@@@@@@  @@@@   @@@@@
    @@@@      @@@@@@@@@                                            @
    @@@@      @@@@@@@@@         @@@@@@     @@@@@@@@@@  @@@@@@@   @@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@  @@@@@@@   @@@@@
    @@@@      @@@@@@@@@         @@@@@@@@ @@@@@@@@@@@@  @@@@@@@  @@@@@@@
    @@@@      @@@@@@@@@@@    @@@  @@@@@@ @@@@@@  @@@@   @@@@@@  @@@@@@@
    @@@@      @@@@@ @@@@@@@@@@@@  @@@@@@ @@@@@@  @@@@   @@@@@@@@@@@@@@@@@
    @@@@      @@@@@   @@@@@@@@@@   @@@@@ @@@@@   @@@@     @@@@@@@@@ @@@@@
     @@@@@   @@@@      @@@@@@@@       @@ @@      @@@@     @@@@@@@     @@@@
     @@@@@@@@@@@@                                            @@@@     @@@@
       @@@@@@@@@                   Corporation             @@@@@       @@@@@
        @@@@@@               San Diego, California         @@@@@       @@@@@


       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.

       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.


*****************************************************************************/

/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: xdebug.h                                                             ::
 * ::                                                                      ::
 * ::                 Debugging control and generation                     ::
 * ::                                                                      ::
 * :: This module provides the include file definitions used for debugging ::
 * :: controlling and generating common debugging output.  By using the    ::
 * :: definitions in this file, your module will be able to provide        ::
 * :: standardized debugging information similar to the other standard     ::
 * :: modules provided by UconX.                                           ::
 * ::                                                                      ::
 * :: In the declaration section of one of your main modules, specify the  ::
 * :: following macro name to declare the debugging control variables for  ::
 * :: your module:                                                         ::
 * ::                                                                      ::
 * ::     DBG_DECL( N, A, B );                                             ::
 * ::                                                                      ::
 * :: where A is the maximum debugging message number used in your module  ::
 * :: (see the DMSG description below), and B is the maximum number of     ::
 * :: trace buffer entries to make, and N is the address of a pointer to   ::
 * :: the name of the module.                                              ::
 * ::                                                                      ::
 * :: Most standard debugging is then performed by using the DMSG macro    ::
 * :: which is used to generate debugging output information and whose     ::
 * :: parameters are as shown:                                             ::
 * ::                                                                      ::
 * ::     DMSG( 1, 2, 3, 4, 5, 6 )                                         ::
 * ::                                                                      ::
 * ::     1 = Debugging message number                                     ::
 * ::     2 = Upstream queue for this message                              ::
 * ::     3 = Primary string (printf format)                               ::
 * ::     4 = Primary debugging trace value                                ::
 * ::     5 = Secondary debug value                                        ::
 * ::     6 = Tertiary debug value                                         ::
 * ::                                                                      ::
 * :: Depending on the settings of the debugging control as specified,     ::
 * :: the DMSG macro will do one or more of the following:                 ::
 * ::     1. Print the string on STDOUT, using parameters 3, 4, and 5 for  ::
 * ::        any substitution points marked in the string by percent signs.::
 * ::        The output goes to the RS232 console port on the board.       ::
 * ::     2. Store the debugging message number and the primary debugging  ::
 * ::        trace value in a circular trace buffer. This trace buffer may ::
 * ::        be examined using the on-board debugger, and is located by the::
 * ::        symbol xdebug_trace for your module.                          ::
 * ::     3. Send an M_IOCTL I_STR UCS_DMSG ioctl upstream containing the  ::
 * ::        string and the values on the queue specified as parameter 2.  ::
 * ::        Note that it is sometimes impossible to associate a particular::
 * ::        queue to the message; in this case, specify 0 and no message  ::
 * ::        will be generated for this output form.                       ::
 * :: An example of using this macro is as follows:                        ::
 * ::     DMSG( 0x32C, RD(q), "modulewsrv routine, queue=%x\n", q, 0, 0 ); ::
 * ::                                                                      ::
 * ::                                                                      ::
 * :: The debugging output and levels are controlled by the following      ::
 * :: macros:                                                              ::
 * ::     DBG_NONE	- Declare before DBG_DECL or any other debugging   ::
 * ::                     definition.  Causes all debugging code to be     ::
 * ::                     omitted from your module.                        ::
 * ::     DBG_ENABLE(N)	- Passed an argument which is zero or a logical OR ::
 * ::                     of one or more of the following values.  Causes  ::
 * ::                     the output of debugging information to be updated::
 * ::                     dynamically to add the specified forms of debug  ::
 * ::                     output for all debug messages whose bit has been ::
 * ::                     set in the control array.                        ::
 * ::                          DBG_PRINTF  - Enables printing of msgs      ::
 * ::                          DBG_TRACE   - Enables trace buffer logging  ::
 * ::                          DBG_MSGS    - Enables upstream sending of   ::
 * ::                                        debug M_IOCTL messages        ::
 * ::     DBG_DISABLE(N)- Disables the specified debugging output.  The    ::
 * ::                     arguments to this macro are the same as to the   ::
 * ::                     DBG_ENABLE macro, and have the opposite effect.  ::
 * ::     DBG_SET(N)	- Enables debugging output for the specified       ::
 * ::                     message number.  Note that this control can only ::
 * ::			  be specified for message numbers in the range of ::
 * ::			  0-65535.  Any messages above this range are      ::
 * ::			  controlled by message number 65535 (0xffff).     ::
 * ::     DBG_CLR(N)	- Disables debugging output for the specified      ::
 * ::                     message number.  See range note above.           ::
 * ::     DBG_UPFUNC	- Function used to send messages upstream; used by ::
 * ::                     the DMSG macro above and the DBG_IOCTL macro.    ::
 * ::                     Default = putq; arguments = (*queue_t, *mblk_t)  ::
 * ::     DBG_MASK(N)   - Defines a mask value for message numbers.  The   ::
 * ::			  message number is and'ed with this mask and if   ::
 * ::			  the result is non-zero (or matches the DBG_CMP   ::
 * ::			  value if that value is non-zero) then the output ::
 * ::			  is enabled for that debugging message number.    ::
 * ::     DBG_CMP(N)    - Defines a value to be compared to the result of  ::
 * ::                     the debugging message number and'ed with the     ::
 * ::                     mask to determine what outputs are to be shown.  ::
 * ::     DBG_DO(C)	- Includes code C when debugging enabled.          ::
 * ::     DBG_IOCTL(m,uq,dq)                                               ::
 * ::			- Determines if the passed downstream message is a ::
 * ::                     debugging control M_IOCTL message, and if so,    ::
 * ::                     modifies the current debugging according to the  ::
 * ::                     message specifications.  After execution of this ::
 * ::                     macro, one of the following conditions may be    ::
 * ::                     observed:                                        ::
 * ::                     1. message pointer is zero: message was a debug  ::
 * ::                        message.  If it was for this module, it has   ::
 * ::                        been handled and an ACK or NAK has been sent  ::
 * ::                        back upstream by calling DBG_UPFUNC(uq, m).   ::
 * ::                        If the message was not for this module, it has::
 * ::                        been sent downstream with putnext(dq, m).     ::
 * ::                     2. message pointer is non-zero: message was not  ::
 * ::                        a debug message and must be handled otherwise ::
 * ::                        by this service procedure.                    ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */

#ifndef _xdebug_h
#define	_xdebug_h


/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: Debugging declarations                                               ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */

/* ........................................................................
 * .                                                                      .
 * . User code declarations                                               .
 * .                                                                      .
 * . The following represent the declarations made in the user's code at  .
 * . the DBG_DECL location.                                               .
 * .                                                                      .
 * ........................................................................
 */

#ifdef DBG_NONE

#define	DBG_DECL(I,N,T)

#else

extern	int    xdebug_ctl;
extern	char   xdebug_msgs[];
extern	int    xdebug_nmsgs;
extern	struct xdebug_trace *xdebug_tcur, *xdebug_tmax;
extern	int    xdebug_dmsg();
extern	char **xdebug_name;
extern  unsigned int xdebug_mask;
extern  unsigned int xdebug_cmp;

#define	DBG_DECL(I,N,T) 					\
int	xdebug_ctl    = 0x0;					\
unsigned int xdebug_mask = 0;					\
unsigned int xdebug_cmp = 0;					\
char	xdebug_msgs[((N)>0xffff ? 0xffff : (N)) /8];		\
int	xdebug_nmsgs = (N);					\
struct  xdebug_trace xdebug_trace[T];				\
struct	xdebug_trace *xdebug_tcur  = &(xdebug_trace[0]);	\
struct	xdebug_trace *xdebug_tmax  = &(xdebug_trace[T]);	\
char  **xdebug_name = I;

#endif

#define	UDB_DBGNULL	('d'<<8)
#define UDB_DBGSET	(UDB_DBGNULL+5)	/* 1st lw = module id; lw N = msg # */
#define UDB_DBGCLR	(UDB_DBGNULL+6)	/* 1st lw = module id; lw N = msg # */
#define UDB_DBGMSG	(UDB_DBGNULL+0xDB)	/* M_PCPROTO upstream 1st lw */


/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: Debugging control                                                    ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */

#define	DBG_DISCARD	0x0	/* Debugging information suppressed */
#define	DBG_PRINTF	0x1	/* Debugging info printed to tty port */
#define	DBG_TRACE	0x2	/* Debugging info logged in trace buffer */
#define	DBG_MSGS	0x4	/* Debugging info sent upstream in msgs */

#ifdef DBG_NONE

#define	DBG_ENABLE(M)
#define	DBG_DISABLE(M)

#else

#define	DBG_ENABLE(M)	{ xdebug_ctl |= M; }
#define	DBG_DISABLE(M)	{ xdebug_ctl &= ~M; }

#endif


/* ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 * ::                                                                      ::
 * :: Debugging definitions                                                ::
 * ::                                                                      ::
 * ::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
 */

/* ........................................................................
 * .                                                                      .
 * . debug_trace buffer structure definition                              .
 * .                                                                      .
 * . The debug_trace structure defines the entries made into the debug    .
 * . trace buffer when tracing is enabled.                                .
 * .                                                                      .
 * ........................................................................
 */

#ifndef DBG_NONE

struct xdebug_trace    		/* Entries into the debug_trace buffer */
{
    unsigned long id;		/* Tracepoint ID */
    unsigned long val;		/* Tracepoint value */
};

extern	struct xdebug_trace xdebug_trace[];

#endif


#ifndef	DBG_UPFUNC
#define	DBG_UPFUNC	putq	/* Function used to send messages upstream */
#endif

/* ........................................................................
 * .                                                                      .
 * . DEBUG macros                                                         .
 * .                                                                      .
 * . These macros are used to set/clear/test the cXd_dbg_msgs control     .
 * . array bits and are used by the other debugging macros and various    .
 * . portions of the code which affect the debugging operation.           .
 * ........................................................................
 */

#ifdef DBG_NONE

#define	DBG_IND(N)    0
#define	DBG_BIT(N)    0
#define	DBG_ISSET(N)  0
#define	DBG_SET(N)
#define	DBG_CLR(N)
#define	DBG_MASK(N)
#define	DBG_CMP(N)
#define	DBG_DO(C)

#else

#define	DBG_IND(N)    (((N) > 0xffff ? 0xffff : (N)) /8)
#define	DBG_BIT(N)    (1 << ((N)%8))
#define	DBG_ISSET(N)  (xdebug_msgs[DBG_IND(N)] &   DBG_BIT(N))
#define	DBG_SET(N)    {xdebug_msgs[DBG_IND(N)] |=  DBG_BIT(N);}
#define	DBG_CLR(N)    {xdebug_msgs[DBG_IND(N)] &= ~DBG_BIT(N);}
#define	DBG_MASK(N)   {xdebug_mask = (N);}
#define	DBG_CMP(N)    {xdebug_cmp = (N);}
#define	DBG_DO(C)     C

#endif

/* ........................................................................
 * .                                                                      .
 * . DMSG macro                                                           .
 * .                                                                      .
 * . This macro is the primary debugging output macro; the first argument .
 * . specifies the message number; if valid in the xdebug_msgs array,     .
 * . the remaining arguments are output as determined by the bits set in  .
 * . the xdebug_ctl variable.                                             .
 * ........................................................................
 */

#ifdef DBG_NONE
#  define	DMSG(N,Q,F,A,B,C)
#else
#  define	DMSG(N,Q,F,A,B,C)	xdebug_dmsg(N, Q, F, A, B, C);
#endif


/* ........................................................................
 * .                                                                      .
 * . DBG_IOCTL macro                                                      .
 * .                                                                      .
 * . This macro is used in the write service routine to check each buffer .
 * . retrieved from the queue with the getq operation for a debugging     .
 * . control M_IOCTL message, and to handle that message if it is.        .
 * ........................................................................
 */

#ifdef DBG_NONE

#define	DBG_IOCTL(M,Q,N)

#else

#define	DBG_IOCTL(M,Q,N) {						\
    register int __i;							\
    register struct iocblk *__p_ioc = (struct iocblk *)(M)->b_rptr;	\
    if( (M)->b_datap->db_type == M_IOCTL &&				\
	(__p_ioc->ioc_cmd == UDB_DBGSET || 				\
	 __p_ioc->ioc_cmd == UDB_DBGCLR) ) {				\
	if( __p_ioc->ioc_count < 4 || __p_ioc->ioc_count % 4 != 0 ) {	\
	    (M)->b_datap->db_type = M_IOCNAK;				\
	    DBG_UPFUNC( (Q), (M) );					\
	} else if( *((int *)(M)->b_cont->b_rptr) != 			\
		   (Q)->q_qinfo->qi_minfo->mi_idnum ) {			\
	    putnext( (N), (M) );					\
	} else {							\
	    for( __i = __p_ioc->ioc_count / 4 - 1; __i > 0; __i-- )	\
		if( __p_ioc->ioc_cmd == UDB_DBGSET ) {			\
		    DBG_SET( ((int *)(M)->b_cont->b_rptr)[i] );		\
		} else {						\
		    DBG_CLR( ((int *)(M)->b_cont->b_rptr)[i] );		\
		}							\
	    (M)->b_datap->db_type = M_IOCACK;				\
	    DBG_UPFUNC( (Q), (M) );					\
	}								\
	(M) = NULL;							\
    }									\
}

#endif

#endif /* _xdebug_h */
