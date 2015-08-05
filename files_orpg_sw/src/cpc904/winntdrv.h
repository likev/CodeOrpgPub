/*   @(#) winntdrv.h 99/12/23 Version 1.11   */

/*
Modification history:
 
Chg Date       Init Description
1.  24-FEB-98  mpb  Internal hooks so layered drivers can chi-chat.
2.  26-MAR-98  mpb  Provide hooks to allow client app to get info from the
                    kernel.
3.  01-JUN-99  mpb  Make an ioctl which will provide the number of
                    controlleres (and what type) are in a system.
4.  06-DEC-99  mpb  Provide hooks so client apps can access the HS
                    Control Register (for debugging purposes).
*/

// Special hooks to the driver in the kernel.
#define GET_HS_INFO  0x1110  // Get Hot Swap control register.
#define SET_HS_INFO  0x1111  // Set Hot Swap control register.

// Erro numbers which are not defined in the NT domain.  These must be kept
// consistant between the driver, the api, nad the client application.  All
// three will include this file.  These error values will allow MPSperror()
// to print out the appropriate string.
#define  MPSERR         0xc0

#define   ECONNRESET    (MPSERR | 19)   /* Connection reset by peer */
#define   ENOBUFS       (MPSERR | 20)   /* No buffer space available */
#define   ETIMEDOUT     (MPSERR | 21)   /* Connection timed out */
#define   ECONNREFUSED  (MPSERR | 22)   /* Connection refused */
#define   ENOSR         (MPSERR | 24)   /* Out of streams resources */
#define   EBADMSG       (MPSERR | 25)   /* Trying to read unreadable message */
#define   EOPENTIMEDOUT (MPSERR | 61)   /* No response to MPSopen */

#define   EWOULDBLOCK       EAGAIN

// These are needed for NT with TCP/IP
#define   EINPROGRESS   36            /* Operation now in progress */
#define   EALREADY      37            /* Operation already in progress */
#define   EISCONN       56            /* Socket is already connected */


// #2
#define KERNEL_INFO  201   // Unique number needed.
#define MAX_STREAMS  256   // Per device (from mps.h).

// Taken from hif's mps.h.  The board will define these.  Be sure
// to change this if the values on the board change.

/* Number of data descriptors each for read and write */
#define NRDESC          64      /* board read, host=write */
#define NWDESC          64      /* board write, host read */

/* Total number of control and data descriptors per region */
#define NUMCONTROL      64

typedef struct nt_stream_info
{
   int      wrnodesc;      /* write failed, no descriptors */
   int      wrdescenable;  /* write q enabled, desc avail */
   int      wrblock;       /* write q blocked by flow control */
   int      wrunblock;     /* write q unblocked by flow control */
   int      rdblock;       /* read q blocked by flow control */
   int      rdunblock;     /* read q unblocked by flow control */
   int      write;         /* number of messages written */
   int      read;          /* number of messages read */
   int      reset;         /* flag: stream was active when board was RESET */
   u_short  wr_flags;      /* downstream flags */
   u_short  rd_flags;      /* upstream flags */
   u_int    sid;           /* our internal stream id */
   u_int    board_sid;     /* corresponding board stream id */

   int      numWriteIrpEntries;  /* Waiting to write into desc region. */
   int      numReadIrpEntries;   /* Waiting for read data from card. */
   
   BOOLEAN  readData;        /* Any data waiting ofr an IRP to claim? */
   BOOLEAN  hipri_readData;  /* Any hipri data waiting ofr an IRP to claim? */
} NTstreamInfo, *PNTstreamInfo;

typedef struct nt_kernel_info
{
   int    devxstate;             /* device state */
   short  numwrbufs;             /* number of write data descriptors */
   short  numrdbufs;             /* number of read data descriptors */
   short  numhctl;               /* number of host control descriptors */
   short  numcctl;               /* number of board control descriptors */

   u_int   unclaimedInterrupts;

   int    hi_water;              /* High Watermark. */
   int    lo_water;              /* Low Watermark.  */

   mps_stats_t  stats;
   int    inthost;
   int    intboard;
   int    intboard_avoided;
   int    invalid_msgs;

   u_int  shmem_base;      /* base of card shared memory on Bus */
   u_int  shmem_size;      /* size of card shared memory */
   u_int  local_addr;      /* base of card local address */
   u_int  desc_region_size;
   u_int  desc_region_offset;

   u_int  host_pend;        /* host interrupt pending */
   u_int  board_pend;       /* board interrupt pending */

   u_int  write_desc_status [ NWDESC ];
   u_int  read_desc_status [ NRDESC ];
   u_int  hctl_status [ NUMCONTROL ];
   u_int  cctl_status [ NUMCONTROL ];

   int  num_streams;
   NTstreamInfo  streams [ MAX_STREAMS ];
} NTkernelInfo, *PNTkernelInfo;

#if defined ( WINNT_EMBEDDED_HOOKS )
// The stuff below is for the API and driver to communicate 
// properly.
#define  MAX_LOAD_DATA  1024

#define  NT_COPEN       0x801
#define  NT_RESET       0x802
#define  NT_PUTBLOCK    0x803
#define  NT_EXEC        0x804
#define  NT_PTBUG       0x805
#define  NT_SYSREP      0x806
#define  NT_GETMSG      0x807
#define  NT_XPUSH       0x808
#define  NT_XPOP        0x809
#define  NT_FIONBIO     0x80A
#define  NT_XSTR        0x80B
#define  NT_XLINK       0x80C
#define  NT_XUNLINK     0x80D
#define  NT_POLL        0x80E
#define  NT_KERNEL      0x80F
#define  NT_CNTLR_INFO  0x810
#define  NT_GET_HS_CSR  0x811
#define  NT_SET_HS_CSR  0x812


#define  IOCTL_PCI334_COPEN    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_COPEN,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_RESET    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_RESET,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_PUTBLOCK    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_PUTBLOCK,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_EXEC    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_EXEC,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_PTBUG    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_PTBUG,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_SYSREP    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_SYSREP,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_GETMSG    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_GETMSG,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_XPUSH    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_XPUSH,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_XPOP    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_XPOP,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_FIONBIO    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_FIONBIO,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_XSTR    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_XSTR,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_XLINK    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_XLINK,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_XUNLINK    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_XUNLINK,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_POLL    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,          \
            NT_POLL,                     \
            METHOD_BUFFERED,              \
            FILE_ANY_ACCESS )

#define  IOCTL_PCI334_KERNEL    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,           \
            NT_KERNEL,                     \
            METHOD_BUFFERED,               \
            FILE_ANY_ACCESS )

#define  IOCTL_CNTLR_INFO    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,        \
            NT_CNTLR_INFO,              \
            METHOD_BUFFERED,            \
            FILE_ANY_ACCESS )

#define  IOCTL_GET_HS_CSR    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,        \
            NT_GET_HS_CSR,              \
            METHOD_BUFFERED,            \
            FILE_ANY_ACCESS )

#define  IOCTL_SET_HS_CSR    CTL_CODE ( \
            FILE_DEVICE_UNKNOWN,        \
            NT_SET_HS_CSR,              \
            METHOD_BUFFERED,            \
            FILE_ANY_ACCESS )


// Error message types from the device driver of the embedded 334 card.
//
//  Values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
#define  NTMPS_STATUS         0xE0000000     // Might need to set the Facility
                                             // code.
#define  NTMPS_EAGAIN         ( NTMPS_STATUS | EAGAIN )
#define  NTMPS_EBADF          ( NTMPS_STATUS | EBADF )
#define  NTMPS_EBADMSG        ( NTMPS_STATUS | EBADMSG )
#define  NTMPS_EINTR          ( NTMPS_STATUS | EINTR )
#define  NTMPS_EINVAL         ( NTMPS_STATUS | EINVAL )
#define  NTMPS_EIO            ( NTMPS_STATUS | EIO )
#define  NTMPS_EMFILE         ( NTMPS_STATUS | EMFILE )
#define  NTMPS_ENFILE         ( NTMPS_STATUS | ENFILE )
#define  NTMPS_ENOBUFS        ( NTMPS_STATUS | ENOBUFS )
#define  NTMPS_ENOSR          ( NTMPS_STATUS | ENOSR )
#define  NTMPS_ENXIO          ( NTMPS_STATUS | ENXIO )
#define  NTMPS_EOPENTIMEDOUT  ( NTMPS_STATUS | EOPENTIMEDOUT )
#define  NTMPS_ECONNRESET     ( NTMPS_STATUS | ECONNRESET )
#define  NTMPS_EEXIST         ( NTMPS_STATUS | EEXIST )
#define  NTMPS_ETIMEDOUT      ( NTMPS_STATUS | ETIMEDOUT )


typedef char*   caddr_t;

// Needed information to simulate a getmsg() call in the NT world.
typedef struct
{
   int     retval;      // Return value (from driver) for getmsg() call.
   int     max_csize;   // #1.
   int     max_dsize;   // #1
   UCSHdr  uhdr;
} getmsg_t;


/*
 * Structure of stream info on host supplied in 
 * the poll arrays.  The fd is for File Descriptor
 * which does not make too much sense in NT since
 * HANDLES and sockets are two different animals,
 * but we keep the name here for continuity with 
 * UNIX.
 */

struct ntpollfd
{
   u_long ctlrNum;
   u_long streamIndex;
   short  events;                   /* events of interest on fd */
   short  revents;                  /* events that occurred on fd */
};


// Poll info t required by the driver.
typedef struct
{
   int nfds;
   int timeout;
   LARGE_INTEGER  reserved;  // Gives the driver some storage space.
   struct ntpollfd fds[];
} ntPollInfo;


// 
// NT device driver needs to know the timeout value since it is in charge of
// ensuring something  will happen in that timeout period (unlike in UNIX where
// the system takes care of that for the ioctl() call.  So keep things a
// generalized (in case of change of OpenData struct) as possible.
//
typedef struct
{
   OpenData  od;
   int       timeout;
} NTopenData, *PNTopenData;

typedef struct
{
   u_long  stream_id;
   u_long  stream_index;
} NTopenInfo;

// #1
//
// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// These items will have to be in a common place where all the drivers in
// the layer can access.  This will be for chit-chat between them.

#define XX_MAX_BUFFER_SIZE           300
#define XX_NO_BUFFER_LIMIT           ((ULONG)(-1))

#define IOCTL_XX_GET_MAX_BUFFER_SIZE            \
        CTL_CODE( FILE_DEVICE_UNKNOWN, 0x8FF,   \
                METHOD_BUFFERED, FILE_ANY_ACCESS )
   
typedef struct _XX_BUFFER_SIZE_INFO
{
   ULONG MaxWriteLength;
   ULONG MaxReadLength;
} XX_BUFFER_SIZE_INFO, *PXX_BUFFER_SIZE_INFO;

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#endif /* WINNT_EMBEDDED_HOOKS */
