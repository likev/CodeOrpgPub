/*   @(#) mpsprom_nvram.h 99/05/17 Version 1.9   */

/*<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:
  
                            Copyright (c) 1992 by
 
               +++    +++                           +++     +++
               +++    +++                           +++     +++
               +++    +++                            +++   +++ 
               +++    +++   +++++     + +    +++   +++++   +++ 
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
               +++    +++ ++++++++ ++++ ++++ ++++  ++++++ +++  
               +++    +++++++   ++ ++++ ++++ ++++  +++ ++++++  
               +++    ++++++      +++++ ++++++++++ +++ +++++   
               +++    ++++++      +++++ ++++++++++ +++  +++    
               +++    ++++++      ++++   +++++++++ +++  +++    
               +++    ++++++                             +     
               +++    ++++++      ++++   +++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++  +++    
               +++    ++++++      +++++ ++++++++ +++++ +++++   
               +++    +++++++   ++ ++++ ++++ +++  ++++ +++++   
               +++    +++ ++++++++ ++++ ++++ +++  +++++++++++  
               +++    +++  +++++++  +++ +++  +++   ++++++ ++++ 
                +++  +++    +++++     + +    +++   ++++++  +++ 
                ++++++++                             +++    +++
                 ++++++         Corporation         ++++    ++++
                  ++++   All the right connections  +++      +++
 
 
       This software is furnished  under  a  license and may be used and
       copied only  in  accordance  with  the  terms of such license and
       with the inclusion of the above copyright notice.   This software
       or any other copies thereof may not be provided or otherwise made
       available to any other person.   No title to and ownership of the
       program is hereby transferred.
 
       The information  in  this  software  is subject to change without
       notice  and  should  not be considered as a commitment by UconX
       Corporation.
   
       UconX Corporation
       San Diego, California

 :<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>:<>*/
/***********************************************************************
Modification Record

26-feb-97	pmt	1. Added the SNMP fields to the NVRAM structure.
08-sep-97	pmt	2. Added the SNTP fields to the NVRAM structure.
19-mar-98	rjp 	3. Added support for powerquicc.
01-JUN-98	mpb 	4. Useful define values to help map out the NVRAM.
15-dec-98       lmm     5. Eliminate use of POWERQUICC
12-may-99	rjp	6. Added support for mps800
***********************************************************************/
 

#ifndef _mpsprom_nvram_
#define _mpsprom_nvram_
 
#define N_MPS_ROUTES       16
#define N_BOOT_CLIENTS     100

#define USER_FLASH_SIZE          1024
#define SYSREP_FREQ_OFFSET       ( USER_FLASH_SIZE - 1 )
#define RESET_PERMISSION_OFFSET  ( USER_FLASH_SIZE - 2 )

typedef struct
{
   u_char	net_addr [N_MPS_ROUTES];
   u_char	gw_addr [N_MPS_ROUTES];
} MPSrtentry;

/* There are several read-write objects in the MIB that should survive
a reboot of the platform for SNMP: name, location, and contact. #1 */ 
#define MAX_SNMP_STRING_LEN	255

typedef struct 	
{
    char snmp_contact[MAX_SNMP_STRING_LEN];
    char snmp_name[MAX_SNMP_STRING_LEN];
    char snmp_location[MAX_SNMP_STRING_LEN];
} SNMP_FIELDS;


struct	NVRAM
{
   int		valid_flash;
   char		password	    [ 65 ];
   bit8		port_mode_select;	/* #12 Select rs232 or rs530	*/
   bit16	obmem_size;
   char		diag_flags          [ 2 ];
   short	tcp_port_number;
   short	udp_port_number;
   short	udp_all_port_number;
   short	diag_tcp_port_number;
   short	diag_udp_port_number;
   char		server_inet_addr    [ 16 ];
   char		server_enet_addr    [ 6 ]; /* preset at the factory    */
   char		logical_server_name [ 65 ];
   char		download_cmdfile    [ 81 ];
   char		client_enet_addr    [ 6 ]; /* filled in from arp reply */
   bit8		client_inet_eol;
   char		client_inet_addr    [N_BOOT_CLIENTS + 1] [ 16 ];
   bit32        host_slave_addr;

/* 16 position switch selects one of 15 (0 thru E) addresses 
   input by the user (F is the host) */

   bit32        address_table [ 15 ];

   bit32        slave_addr_mode;
#define            A32S         0
#define            A24S         1

   bit32        A24_size;
   bit32        A32_size;
   char		user_defined [ USER_FLASH_SIZE ];
   char         bootp_config [2];       /* Flag initting via bootp      */
   char 	gw_ipaddr [20];
   MPSrtentry 	gways [16];
   int 		gway_index;
   char		server_inet_addr2 [ 16 ];
   SNMP_FIELDS  snmp_fields;		/* Save area for SNMP fields #1 */
   char		ntpserver_inet_addr [ 16 ];
   bit32	ntp_drift;
   bit8		ntpserver_freq;
};

/* The startup system parameter block and user defined flash prom in RAM */

#ifdef OBSOLETE  /* #5 - holdover from Atlas development */ 
/* 
* Phoney.
*/
/* Starting execution address for this board */
 
struct  X_ADDR
{
   bit32        valid_word;
   bit32        ( *exec_addr ) ( );
};
 
struct DNL_TYPE
{
   struct       X_ADDR  x_addr;
   bit32        dxr;            /* download exchange region */
#define         ilevel  dxr     /* interrupt the host at this level */
#define         laddr   dxr     /* begin loading at this on-board address */
#define         lsize   dxr     /* size of this download block */
#define         iaddr   dxr     /* init the on-board system at this address */
#define         breply  dxr     /* board's reply to the command */
#define         ixchg   dxr     /* IN HOST'S MEMORY ONLY */
#define PROM_ACK        1
#define PROM_NAK        2
   bit32        ivect;          /* interrupt the host with this vector */
   bit32        naddr;          /* address where next block of data goes */
   char         dblock [ 1024 ];/* buffer where data is host data is found */
};
#endif				/* #6 					*/

#ifdef MPS800			/* #6 					*/
#define		SYS_U_BLOCK	(SHMEM_BASE+0x3200)
#else 
#define		SYS_U_BLOCK	(SHMEM_BASE+0x1500)
#endif

struct	sys_u_block
{
#define			SYS_BLOCK_SIZE	16
   bit32		sys_block [ SYS_BLOCK_SIZE  ];
#define			SYS_DIAG_FLAG	0 
   bit8			u_block   [ USER_FLASH_SIZE ];
   struct DNL_TYPE	dnlblock;
};

#endif /* _mpsprom_nvram_ */
