/*   @(#)sx25.h	1.1	07 Jul 1998	*/
/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Ian Lartey
 *
 * sx25.h of snet module
 *
 * SpiderX25
 * @(#)$Id: sx25.h,v 1.1 2000/02/25 17:15:01 john Exp $
 * 
 * SpiderX25 Release 8
 */



/*
 *             prototypes for sx25 [libsx25.a]
 */

/* any.c */
extern char *any(char *cp, char *match);

/* equalx25.c */
extern int equalx25(struct xaddrf *x1, struct xaddrf *x2);
extern int x25add_equal(char *s1, char *s2, unsigned int len);

/* getaddrbyid.c */
extern struct xaddrf *getaddrbyid(uint32 snid);

/* getconfent.c */
extern void setconfent(char *confname, int f);
extern void endconfent(char *confname);
extern struct confsubnet *getconfsubent(char *confname);
extern struct confsubnet *confsubentdup(struct confsubnet *src_p);

/* getconfintent.c */
extern void setconfintent(char *confname, int f);
extern void endconfintent(void);
extern struct confinterface *getconfifacent(char *confname);
extern struct confinterface *confifacentdup(struct confinterface *src_p);

/* getintbysnid.c */
extern struct confinterface *getintbysnid(char *confname, uint32 snid);
extern struct confinterface *getnextintbysnid(char *confname, uint32 snid);

/* getnettype.c */
extern int getnettype(unsigned char *snid);

/* getpadbyaddr.c */
extern struct padent *getpadbyaddr(char *addr);

/* getpadbystr.c */
extern char *getpadbystr(char *s);

/* getpadent.c */
extern void setpadent(int f);
extern void endpadent(void);
extern struct padent *getpadent(void);

/* getsubnetbyi.c */
extern struct subnetent *getsubnetbyid(uint32 snid);

/* getsubnetbyn.c */
extern struct subnetent *getsubnetbyname(char *name);

/* getsubnetent.c */
extern void setsubnetent(int f);
extern void endsubnetent(void);
extern struct subnetent *getsubnetent(void);

/* getxhostbyad.c */
extern struct xhostent *getxhostbyaddr(char *addr, int len, int type);

/* getxhostbyna.c */
extern struct xhostent *getxhostbyname(char *name);

/* getxhostent.c */
extern void setxhostent(int f);
extern void endxhostent(void);
extern struct xhostent *getxhostent(void);

/* padtos.c */
extern int padtos(struct padent *p, char *strp);

/* snidtox25.c */
extern uint32 snidtox25(unsigned char *snid);

/* stox25.c */
extern int stox25(unsigned char *cp, struct xaddrf *xad, int lookup);

/* x25tos.c */
extern int x25tos(struct xaddrf *xad, unsigned char *cp, int lookup);

/* x25tosnid.c */
extern int x25tosnid(uint32 snid, unsigned char *str_snid);
