/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/10/30 12:30:22 $
 * $Id: query_product_print.h,v 1.3 2012/10/30 12:30:22 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */  

#ifndef	QUERY_PROD_PRINT_H
#define	QUERY_PROD_PRINT_H

/* Include files */

#include <rpg_ldm.h>

/* Defines/enums */

#define FORMAT_HDR_NONE                 0x00000000
#define FORMAT_PROD_HDR_TIME            0x00000001
#define FORMAT_PROD_HDR_KEY             0x00000002
#define FORMAT_PROD_HDR_FLAGS           0x00000004
#define FORMAT_PROD_HDR_FEEDTYPE        0x00000008
#define FORMAT_PROD_HDR_SEQNUM          0x00000010
#define FORMAT_PROD_HDR_LENGTH          0x00000020
#define FORMAT_PROD_HDR_SPARE148        0x00000040
#define FORMAT_PROD_HDR_SPARE150        0x00000080
#define FORMAT_PROD_HDR_SPARE152        0x00000100
#define FORMAT_PROD_HDR_SPARE154        0x00000200
/* place holder for future use          0x00000400 */
/* place holder for future use          0x00000800 */
/* place holder for future use          0x00001000 */
/* place holder for future use          0x00002000 */
/* place holder for future use          0x00004000 */
/* place holder for future use          0x00008000 */
#define FORMAT_PROD_HDR_ALL             0x0000ffff
#define FORMAT_MSG_HDR_CODE             0x00010000
#define FORMAT_MSG_HDR_SIZE             0x00020000
#define FORMAT_MSG_HDR_TIME             0x00040000
#define FORMAT_MSG_HDR_SERVERMASK       0x00080000
#define FORMAT_MSG_HDR_BUILD            0x00100000
#define FORMAT_MSG_HDR_ICAO             0x00200000
#define FORMAT_MSG_HDR_FLAGS            0x00400000
#define FORMAT_MSG_HDR_SIZEU            0x00800000
#define FORMAT_MSG_HDR_SEGNUM           0x01000000
#define FORMAT_MSG_HDR_NUMSEGS          0x02000000
#define FORMAT_MSG_HDR_WMO_SIZE         0x04000000
#define FORMAT_MSG_HDR_SPARE38          0x08000000
#define FORMAT_MSG_HDR_SPARE40          0x10000000
#define FORMAT_MSG_HDR_SPARE42          0x20000000
#define FORMAT_MSG_HDR_SPARE44          0x40000000
#define FORMAT_MSG_HDR_SPARE46          0x80000000
#define FORMAT_MSG_HDR_ALL              0xffff0000

/* Structures */

/* Static/global variables */

/* Function prototypes */

void Set_debug_mode( int );
int  Get_debug_mode();
void Print_debug( const char *, ... );
void Print_error( const char *, ... );
void Print_out( const char *, ... );
void Print_RPG_LDM_prod_hdr( char *, int );
void Print_RPG_LDM_msg_hdr( char *, int );

#endif
