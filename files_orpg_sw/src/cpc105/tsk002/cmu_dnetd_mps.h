/*   @(#) mps.h 99/11/02 Version 1.4   */

/*
 *  STREAMS Network Daemon - DAG Definitions
 *
 *  Copyright (c) 1988-1995 Spider Software Limited
 *
 *  This Source Code is furnished under Licence, and may not be
 *  copied or distributed without express written agreement.
 *
 *  Written by Mark Valentine; CONTROL additions by David Fraser.
 *
 *  Made in Scotland.
 *
 *  @(#)$Id: cmu_dnetd_mps.h,v 1.1 2000/10/20 15:20:09 jing Exp $
 */

/*
Modification history:
 
Chg Date       Init Description
1.   6-MAY-98  rjp  Modified to support UconX api open.
*/
 

/*
 *  The following two data structures, when built from the specification
 *  obtained from the configuration file, describe the STREAMS structure
 *  in terms of a directed acyclic graph whoses nodes are individual
 *  modules and devices.
 *
 *  The first data structure holds a table (actually a linked list) of
 *  information about each module/driver, i.e. its name and type.  A
 *  pointer to the module's node in the directed graph is also kept in
 *  this table.
 *
 *                +------------+
 *    ==--------->|id|type|name|
 *                |------------|
 *              .-|next | graph|----> graph nodes
 *              | +------------+
 *              `-----------------.
 *                +------------+  |
 *                |id|type|name|<-'
 *                |------------|
 *              .-|next | graph|----> graph nodes
 *              | +------------+
 *              v
 *
 *  The second is a representation of the graph itself.  The edges of
 *  the graph represent downstream links.  Associated with each link
 *  is a set of control information to be sent to the *upstream* module/
 *  driver when the link/push is completed.
 *
 *  A file descriptor is associated with each link for internal use by
 *  the STREAMS construction code.
 *
 *                +------+
 *                | inst |
 *                |------|  .-> module table entry
 *                |module|--'
 *    ==--------->|------|     +-------+     +----+   +----+
 *                | links|--.  |control|---->|F(a)|-->|F(a)|
 *                +------+  `->|stream |-.   +----+   +----+ 
 *                             |next o | |
 *                             +-----|-+ `-> downstream node
 *                                   v
 *                             +-------+     +----+   +----+
 *                             |control|---->|F(a)|-->|F(a)|
 *                             |stream |-.   +----+   +----+ 
 *                             |next o | |
 *                             +-----|-+ `-> downstream node
 *                                   v
 *
 *  A linked list keeps track of the stream heads.
 */

struct module		   		/* per module/driver information */
{
	char		*id;		/* identifying string */
	int		type;		/* module/driver flags */
	char		*name;		/* module/device name */
	struct node	*graph;		/* link to instantiations in graph */
	struct module	*next;		/* linked list connection */
#ifdef CONTROL
	struct info	*first_control;
	struct info	*last_control;
#endif
	/*
	* #1 Add UconX api support
	*/
        char            *server;        /* server name */
        char            *cnum;          /* controller number */
        char            *service;       /* transport service name */
        char            *protocol;      /* protocol name from commfig */
};

/* flags for module.type */

#define DRIVER		0x01
#define MODULE		0x02
#define CLONE		0x04
#ifdef CONTROL
#define CONTROL_TBD 	0x08	/* Not a type - but indicates special action */
#endif
#define TYPE_NONBLOCK	0x10	/* #1 Change spelling to avoid conflict	*/

struct info		/* control information sent when making links */
{
	int		(*function)();	/* function which does it */
	int		argc;		/* argument count */
	char		**argv;		/* argument list */
	struct info	*next;		/* next in chain */
};

struct edge		/* edge in directed acyclic graph */
{
	struct node	*stream;	/* downstream node */
	struct info	*control;	/* control instructions */
	int		fd;		/* downstream file descriptor */
	int		muxid;		/* muxid from I_LINK */
	struct edge	*next;		/* next in chain */
};

struct node		/* node in directed acyclic graph */
{
	struct module	*module;	/* pointer to module info */
	int		instant;	/* module instantation */
	struct node	*next;		/* next instantation */
	struct edge	*links;		/* chain of downstream links */
};

struct chain		/* chained list of nodes */
{
	struct node	*chain;		/* node */
	struct chain	*next;		/* linked list pointer */
#ifdef DEMOLISH
	int		fd;		/* file descriptor */
#endif
};

extern struct module	*modtab;	/* module table */
extern struct chain	*heads;		/* list of stream heads */

#define streq(s1, s2)	(strcmp((s1), (s2)) == 0)

extern char	*memory(), *mfree();
extern char	*array();
extern char	*copy();

struct list		/* generic linked list type */
{
	char		*data;		/* data pointer */
	struct list	*next;		/* next link in list */
};

typedef struct list	*list_t;	/* linked list pointer */

#define L_NULL	((struct list *) 0)

extern list_t	mklist();

#ifndef VXWORKS
#ifdef ANSI_C
void exit_program ( int );
#else
void exit_program ( );
#endif /* ANSI_C */
#endif /* !VXWORKS */


/*
* Provide a structure definition for the parameters required for MPSopen.
*/
typedef struct {
    char                serverName [MAXSERVERLEN];
    char                serviceName [MAXSERVNAMELEN];
    int                 ctlrNumber;
    char                protoName [MAXPROTOLEN];
    int                 device;
} UCONX_DEVICE;
 


int doOpen (UCONX_DEVICE *);
int doClose (int);
int doPush (int, char *);
int doLink (int, int);
int doExpmux (int, int);
int uconx_device_create (UCONX_DEVICE *, char *, char *, char *, char *, char *);
int uconx_device_validate (char *, char *, char *, char *, char *);

