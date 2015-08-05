/*   @(#)xnetdb.h	1.1	07 Jul 1998	*/
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * xnetdb.h of include module
 *
 * SpiderX25
 * @(#)$Id: xnetdb.h,v 1.1 2000/02/25 17:15:37 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define MAXXADDRSIZE	100
#define	CCITT_X25	1
#define MAXLINE		256
#define MAXCONFSIZE     100
#define MAX_TCLASS      15
#define MIN_TCLASS      0

/* Definitions of max and min packet and window sizes */
#define MAX_PKT		12
#define MIN_PKT		4
#define MAX_WND		127
#define MIN_WND		1


/*
 *  Defines for x25 getxhost family
 */
 
#define ANY	0
#define LAN	1
#define	W80	2
#define	W84	3
#define	W88	4

#define IFACE   "interface"

/*
 *  Database Files
 */


#ifndef X25CONF
#define	X25CONF	"/etc/x25conf"
#endif
#ifndef SUBNETS
#define	SUBNETS	"/etc/subnetworks"
#endif
#ifndef PADMAP
#define	PADMAP	"/etc/padmapconf"
#endif
#ifndef XHOSTS
#define	XHOSTS	"/etc/xhosts"
#endif
#ifndef X25ACT
#define X25ACT  "/etc/x25conf_a"
#endif

/*
 *  Structures returned by x25 database library.
 *  All addresses are supplied in host order, and
 *  returned in network order (suitable for use in
 *  system calls).
 */
 
struct	xhostent
{
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	*h_addr;	/* pointer to a single address */
};



/*
 *  Defines for pad getpad family
 */

#define MAXCUDFSIZE	124

/*
 *  Structure used by pad database.
 */

struct	padent
{
	struct xaddrf		xaddr;
	unsigned char		x29;
	struct qosformat	qos;
	unsigned char 		cud [MAXCUDFSIZE + 1];
};

/*
 *  Structure used by X.25 information data base
 */

struct  xinfo
{
	struct xaddrf           xaddr;
	unsigned char           x29;
	struct qosformat        qos;
	unsigned char           cud [MAXCUDFSIZE];
};

/*
 *  Structure used by Subnetwork database.
 */

struct  subnetent
{
	struct  xaddrf xaddr;   /* X.25 address */
	char    *alias;         /* subnetwork alias */
	char    *descrip;       /* subnetwork description */
};

struct  confsubnet
{
	char            *dev_type;      /* Type of entry */
	uint32          snid;           /* Subnet id */
	char            *friendly_name; /* subnetwork alias */
	char            *net_type;      /* Type of network ie WAN80 */
	char            *x25reg;	/* Class of lower x25 interface */
	char            *x25_template;  /* level 3 template file */
	char            *mlp_template;  /* mlp template file */
	char            *x25addr;
	char            *nsap;
	char            *descrip;       /* subnetwork description */
};

struct  confinterface
{
	char            *dev_type;      /* Type of entry,  */
	uint32          snid;           /* Subnet id */
	char            *dev_name;      /* Device name */
	uint32          llsnid; /* link level subnetwork id */
	char            *lreg;          /* link class */
	char            *dl_template;   /* link layer template */
	char		*wan_template;
	char		*wanmap_file;
	int             priority;
	int             board_number;
	int             line_number;
};

/* Define whether X.25 address(es) should be checked against subnetwork type  */

#define ADDR_CHECK	1
#define NO_ADDR_CHECK	0


extern void		endsubnetent();
extern void		endpadent();
extern void		endxhostent();
extern int		equalx25();
extern struct xaddrf	*getaddrbyid();
extern struct xhostent	*getxhostbyaddr();
extern struct xhostent	*getxhostbyname();
extern int		getnettype();
extern struct padent	*getpadbyaddr();
extern char		*getpadbystr();
extern struct padent	*getpadent();
extern struct subnetent	*getsubnetent();
extern struct subnetent	*getsubnetbyid();
extern struct subnetent	*getsubnetbyname();
extern struct xhostent	*getxhostent();
extern struct xhostent	*getxhostbyaddr();
extern struct xhostent	*getxhostbyname();
extern struct xhostent	*getxhostent();
extern int		padtos();
extern int		x25add_equal();
extern void		setpadent();
extern void		setsubnetent();
extern void		setxhostent();
extern int		stox25();
extern int		x25tos();
extern int              x25tosnid();
extern uint32           snidtox25();

