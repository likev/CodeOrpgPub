/*   @(#)ixe_ptypes.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * ixe_ptypes.h of snet module
 *
 * SpiderX25
 * @(#)$Id: ixe_ptypes.h,v 1.1 2000/02/25 17:14:25 john Exp $
 * 
 * SpiderX25 Release 8
 */

#ifndef _IXE_PTYPES_H_
#define _IXE_PTYPES_H_

	/* ixe.c */

void ixreg_init(void);
int ix_timer(void);
IX_CON *claim_ixcon(IX_NETENT *, uint32);
void flush_ixcon(IX_CON *);
IX_CON *next_ixcon(void);
IX_CON *get_ixcon(int, uint32);
IX_NETENT *find_ixnet(struct xaddrf *);
IX_NETENT *get_ixnet(queue_t *, uint32);
IX_CON *claim_pvcconn(uint32, unsigned short, IX_NETENT *);
IX_CON *get_pvcconn(struct xaddrf *, uint32);
int make_pvcs(unsigned short, unsigned short, uint32, IX_NETENT *);
void ixe_detach_pvcs(unsigned short, unsigned short, uint32);
void unreg_net(uint32);
#ifdef DEBUG
int put_inet(uint32);
int print_x25(struct xaddrf *);
#endif

	/* ixe_addr.c */

void ixad_init(void);
void ixad_reset(void);
int ixad_ent(struct ixe_ent *, unsigned char);
IX_ADDR *ixad_get(uint32);
IX_ADDR *x25ad_get(struct xaddrf *);
int ixad_del(uint32);
int ixad_dump(queue_t *q, mblk_t *);
int xaddrf_cmp(struct xaddrf *, struct xaddrf *);
void ixad_ddn_ent(uint32, IX_NETENT *);

	/* ixe_snmp.c */

void addr_to_str(unsigned char *, unsigned int, unsigned char *);
int ixe_setall_mib_mioxPle(struct mib_mioxPleEntry *);
int ixe_set_mib_mioxPle(struct var_info *, struct mib_mioxPleEntry *);
int ixe_test_mib_mioxPle(struct var_info *, struct mib_mioxPleEntry *);
int ixe_getnext_mib_mioxPeerEnc(struct mib_mioxPeerEncEntry *);
int ixe_get_mib_mioxPeerEnc(struct mib_mioxPeerEncEntry *);
int ixe_getnext_mib_mioxPeer(struct mib_mioxPeerEntry *);
int ixe_get_mib_mioxPeer(struct mib_mioxPeerEntry *);
int ixe_getnext_mib_mioxPle(struct mib_mioxPleEntry *);
int ixe_get_mib_mioxPle(struct mib_mioxPleEntry *);

	/* ixe_util.c */

mblk_t * make_conreq(IX_ADDR *, IX_NETENT *, IX_CON *);
mblk_t *make_conresp(IX_CON *);
void negotiate(IX_ADDR *, struct xcallf *, IX_CON *, struct xccnff *);
mblk_t *make_disreq(int, unsigned char);
mblk_t *make_lisreq(mblk_t *);
mblk_t *encapsulate(mblk_t *);
void handle_exp(queue_t *, mblk_t *, unsigned char, unsigned char);
mblk_t *make_fragresp(unsigned int);
int ip_to_ddn(uint32 ip_addr, struct xaddrf *);
mblk_t *make_attreq(uint32, unsigned short, unsigned int);
unsigned short extract_lci(struct lsapformat);
mblk_t *make_circreq(void);
mblk_t *make_cprefreq(struct ixe_ent *);
mblk_t *make_cpunrefreq(unsigned int);

#endif /* _IXE_PTYPES_H_ */
