/*   @(#) report.h 99/12/23 Version 1.5   */

/*<-------------------------------------------------------------------------
|
|                           Copyright (c) 1997 by
|
|              +++    +++                           +++     +++
|              +++    +++                           +++     +++
|              +++    +++                            +++   +++
|              +++    +++   +++++     + +    +++   +++++   +++
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++
|              +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++
|              +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++
|              +++    ++++++      +++++ ++++++++++ +++ +++++
|              +++    ++++++      +++++ ++++++++++ +++  +++
|              +++    ++++++      ++++   +++++++++ +++  +++
|              +++    ++++++                             +
|              +++    ++++++      ++++   +++++++ +++++  +++
|              +++    ++++++      +++++ ++++++++ +++++  +++
|              +++    ++++++      +++++ ++++++++ +++++ +++++
|              +++    +++++++   ++ ++++ ++++ +++  ++++ +++++
|              +++    +++ ++++++++ ++++ ++++ +++  +++++++++++
|              +++    +++  +++++++  +++ +++  +++   ++++++ ++++
|               +++  +++    +++++     + +    +++   ++++++  +++
|               ++++++++                             +++    +++
|                ++++++         Corporation         ++++    ++++
|                 ++++   All the right connections  +++      +++
|
|
|      This software is furnished  under  a  license and may be used and
|      copied only  in  accordance  with  the  terms of such license and
|      with the inclusion of the above copyright notice.   This software
|      or any other copies thereof may not be provided or otherwise made
|      available to any other person.   No title to and ownership of the
|      program is hereby transferred.
|
|      The information  in  this  software  is subject to change without
|      notice  and  should  not be considered as a commitment by UconX
|      Corporation.
|
|      UconX Corporation
|      San Diego, California
|
v
-------------------------------------------------------------------------->*/
 
/*
Modification history:
 
Chg Date       Init Description
1.  31-MAR-97  lmm  Added number of blocks for each class
2.  02-JUL-98  lmm  Added max number of mbufs allocated
3.  22-MAR-98  lmm  Added defines and structs to support stream dump ioctl
*/

/* System Report info for reporting to clients in ioctl */

#define SYSREP             (MPS | 0x83)
#define STRDUMP            (MPS | 0x84)		/* #3 */

typedef struct
{
   int bnum;			/* board number */	
   int nclass;			/* number of data block classes */
   int dbsize [MAXCLASS];	/* sizes for each class */
   int dbnum  [MAXCLASS];	/* number blks for each class */	/* #1 */
   alcdat dblk[MAXCLASS];	/* data block class allocation */
   alcdat mblock;		/* message block allocation data */
   alcdat dblock;		/* aggregate data block allocation */
   int memory_cur;		/* current memory avail */
   int memory_min;		/* min memory ever avail */

   /* remainder of information available for servers only! */
   int mbuflist_cur;		/* current mbufs in use */
   int mbuflist_min;		/* min mbufs ever available */ 
   int mbuflist_max;		/* max mbufs bufs allocated */		/* #2 */
   int mcllist_cur;		/* current cluster bufs in use */
   int mcllist_min;		/* min cluster bufs ever available */
   int mcllist_max;		/* max cluster bufs allocated */

} mps_sysrep_t;


/* #3 - Added defines and structs below to support stream dump ioctl */

typedef struct
{
   int bnum;			/* board number */	
} mps_strdump_t;

typedef struct
{
   int  rectype;                    /* record type */
#define STREAM_RECORD      0
#define TCP_STREAM_RECORD  1
#define QUEUE_RECORD       2
#define EOD_RECORD        -1
   int  sm_sid;                     /* stream id                       */
   int  h_sid;                      /* host stream id (TCP socket id)  */
   int  p_smd;                      /* ptr to stream data              */
   int  sm_rq;                      /* stream head read queue pointer  */
   int  sm_wq;                      /* stream head write queue pointer */
   int  sm_state;                   /* stream state/flags              */
   int  sm_ustate;                  /* Upstream state (embedded)       */
                                    /*    (Socket table ptr for TCP)   */
   int  sm_dstate;                  /* Downstream state                */
                                    /*    (Socket state for TCP)       */
   int  writes;                     /* number of writes on stream      */
   int  reads;                      /* number of reads on stream       */

   /* Remaining fields apply to embedded cards only */
   int  wrblock;                    /* times host blocked card         */
   int  wrunblock;                  /* times host unblocked card       */
   int  wrnodesc;                   /* times no data descriptors       */
   int  wrdescenable;               /* times reenabled waiting for ctl */
   int  rdblock;                    /* times card blocked host         */
   int  rdunblock;                  /* times card unblocked host       */
} mps_stream_rec_t;

#define MAX_MODNAME 11 

typedef struct
{
   int  rectype;                    /* record type                     */
   int  lowermux;                   /* if = 1, lower mux queues        */ 

   int  rq;                         /* addr of read queue              */
   char rq_modname[MAX_MODNAME+1];  /* rq module name                  */ 
   int  rq_ptr;                     /* rq private data structure       */
   int  rq_hiwat;                   /* rq high water mark              */
   int  rq_lowat;                   /* rq low water mark               */
   int  rq_count;                   /* rq number of bytes in queue     */
   int  rq_size;                    /* rq number of msgs in queue      */
   int  rq_flag;                    /* rq misc state flags             */
   int  rq_state;                   /* rq scheduling state             */

   int  wq;                         /* addr of write queue             */
   char wq_modname[MAX_MODNAME+1];  /* wq module name                  */ 
   int  wq_ptr;                     /* wq private data structure       */
   int  wq_hiwat;                   /* wq high water mark              */
   int  wq_lowat;                   /* wq low water mark               */
   int  wq_count;                   /* wq number of bytes in queue     */
   int  wq_size;                    /* wq number of msgs in queue      */
   int  wq_flag;                    /* wq misc state flags             */
   int  wq_state;                   /* wq scheduling state             */
} mps_queue_rec_t;
