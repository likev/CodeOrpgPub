/*   @(#)x25_ptypes.h	1.2	03/25/99	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * x25_ptypes.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x25_ptypes.h,v 1.2 2000/11/27 20:51:08 john Exp $
 * 
 * SpiderX25 Release 8
 */

/*
Modification history:

Chg Date       Init Description
1.  25-mar-99  lmm  Changed prototype for memset to avoid compiler warnings
2.  21-nov-00  djb  Made res_PVCs global for user restart enhancement
*/

		/* Prototype declarations for x25 driver */

#ifndef _X25_PTYPES_H_
#define _X25_PTYPES_H_

	/* x25_snmp.c */

int x25_get_mib_if(struct mib_ifEntry FAR * );
struct x25vcformat *if_to_xvc (unsigned int, unsigned int);
int  x25_get_mib_x25Stat (struct mib_x25StatEntry *);
int  x25_getnext_mib_x25Stat (struct mib_x25StatEntry *);
int  x25_get_mib_x25Oper (struct mib_x25OperEntry *);
int  x25_getnext_mib_x25Oper (struct mib_x25OperEntry *);
int  x25_get_mib_x25Circuit (struct mib_x25CircuitEntry *);
int  x25_getnext_mib_x25Circuit (struct mib_x25CircuitEntry *);
int  x25_get_mib_x25ClearedCircuit (struct mib_x25ClearedCircuitEntry *);
int  x25_getnext_mib_x25ClearedCircuit (struct mib_x25ClearedCircuitEntry *);
int  x25_set_mib_x25 (struct mib_x25 *);
int  x25_get_mib_x25CallParmEntry (struct mib_x25CallParmEntry *);
int  x25_getnext_mib_x25CallParmEntry (struct mib_x25CallParmEntry *);
void new_clr_entry (struct x25vcformat *, unsigned char, unsigned char);
int  unref_cp_entry (unsigned int);
int  snmp_test_cp (struct var_info *, struct mib_x25CallParmEntry *);
void set_oper_cp_defaults (struct mib_x25CallParmEntry *, struct wlcfg *);
int  snmp_set_cp (struct var_info *, struct mib_x25CallParmEntry *);
int  create_reref_cp_entry (unsigned int, struct mib_x25CallParmEntry *);
void make_circ_cp_entry (struct x25vcformat FAR *,
        struct mib_x25CallParmEntry *, mblk_t *, int, unsigned char);
void init_cleared_circ_table (void);
void x25_reg_pvcs (uint32, uint32);

	/* x25_stream.c */

int  x25lwput (queue_t *, mblk_t *);

	/* x25aux.c */

void init_lsap (struct lsformat *);
struct lsformat FAR * sn_to_lsap (uint32);
int  authenticate_id (unsigned char *, unsigned char *);
void check_open_conns (void);
int  buffs_status (void);
#if 0
void memset (char FAR *, int, int);
#endif
void *memset ();				/* #1 */
int  x25cmp (char FAR *, char FAR *, int);
void delete_NRS_request (void);
void system_error (int);
void xpurgeQ (struct Q FAR *);
void ON (struct Q *, mblk_t *);
mblk_t * OFF (struct Q *);
void purge_listens (int);
void queue_listen (mblk_t *, int);
unsigned char net_domain (struct wlcfg *);
int sn_to_cfg_index (uint32);
struct wlcfg * sn_to_cfg (uint32);
struct xsndbf * map_snident (uint32);
unsigned char next_Dreq (struct x25vcformat FAR *);
void  map_dbit_control (struct x25vcformat FAR *);
int  qos_available (struct x25vcformat FAR *, int);
int  to_Lregistering (struct lsformat FAR *);
void to_Lconnected (struct lsformat FAR *);
void to_P4D1 (struct x25vcformat FAR *, int);
void xfreedb (struct x25vcformat FAR *);
void deqvc (struct lsformat FAR *, struct x25vcformat FAR *);
void dec_ls_concount (struct lsformat FAR *, struct x25vcformat FAR *);
void xuser_gone (struct x25vcformat FAR *,unsigned char);
void CRs_pending (struct lsformat FAR *);
void dis_lsap_and_vcs (struct lsformat FAR *, unsigned char, unsigned char);
void close_SVCs (struct lsformat FAR *, unsigned char, unsigned char, int);
void P4_FMT_error (struct x25vcformat FAR *, unsigned char);
void state_matrix (struct x25vcformat FAR *, unsigned char);
void ERR_rstU (struct x25vcformat FAR *, unsigned short);
void ERR_clrU (struct x25vcformat FAR *, unsigned short);
void ERR_clrnoU (struct x25vcformat FAR *, unsigned short);
void rst_procedure (struct x25vcformat FAR *, unsigned char, unsigned char,
	unsigned char, unsigned char, unsigned char);
void clr_procedure (struct x25vcformat FAR *,unsigned char, unsigned char,
	unsigned char, unsigned char, unsigned char);
void flush_user (struct x25vcformat FAR *, int);
void tidy_user_side (struct x25vcformat FAR *);
struct lsformat FAR * x25alloclsap (void);
struct x25vcformat FAR * x25allocvc (int *);
void slot_in (struct lsformat FAR *, struct x25vcformat FAR *);
int  assign_lci ( struct x25vcformat FAR *);
struct x25vcformat FAR * lcitovc (struct lsformat FAR *, unsigned short, int);
void fetch_NUI_fac (struct x25vcformat FAR *);
int  nuifac_dump (queue_t *, mblk_t *);
int  nuifac_del (unsigned char FAR *, unsigned char);
NUI_ADDR * nuifac_get (unsigned char FAR *, unsigned char);
int  nuifac_put (struct nui_put FAR *, unsigned char);
void nuifac_init (void);
int  vc_in_range (unsigned short, int *, struct wlcfg *, int *);
int  wildcard_search(char *, int, char *, int);
struct x25vcformat FAR *x25allocvc(int *);
void setup_offset(struct x25vcformat FAR *);
void x25freevc(struct x25vcformat *, struct lsformat FAR *);
int allocxcfg();
void freexcfg(uint32);
void freelsap(struct lsformat *);
void bcd_to_str(char *, unsigned char *, int);
int wildcard_search( char *, int, char *, int);

	/* x25clock.c */

void  l_examtimer (struct lsformat FAR *);
void  x_examtimer (struct x25vcformat FAR *);
void stop_ls_timers (struct lsformat FAR *);
void stop_vc_timers (struct x25vcformat FAR *);
void start_random_timer (struct lsformat FAR *);
void startT23 (struct x25vcformat FAR *);
void startT22 (struct x25vcformat FAR *);

	/* x25dlpi.c */

void x25_ioc_ack (struct lsformat *, int);

	/* x25fac.c */

void fix_def_facilities (struct x25vcformat FAR *, int);
void OK_wneg (unsigned char, unsigned char, struct x25vcformat FAR *);
void OK_pneg (unsigned char, unsigned char, struct x25vcformat FAR *);
void map_tclass (struct x25vcformat FAR *, int, int);
int  pkt_details (struct lsformat FAR *, struct x25vcformat FAR *,
	unsigned char, mblk_t *, int);
int  get_reg_pkt_details (mblk_t *, unsigned char, struct reg_facilities *);

	/* x25init.c */

void x25tick (void);
void x25lscall (struct lsformat FAR *);
void x25vccall (struct x25vcformat FAR *);
void x25clock (void);
int x25init (void);
int  x25release (void);

	/* x25l2.c */

int  dlpi_rxll (queue_t *, mblk_t *);
void l_exam_user (struct lsformat  FAR *);
void L2_disind (struct lsformat FAR *);
void unhook_PVCs (struct lsformat FAR *, int);
#ifdef USER_RESTART /* #2 */
extern void res_PVCs(struct lsformat FAR *, unsigned char, unsigned char);
#endif /* USER_RESTART */

	/* x25lower.c */

void x25_trace (queue_t *, struct lsformat FAR *, mblk_t *, int);
struct xsndbf *allocxsndb (void);
void LL_enable (struct lsformat FAR *);
int  LL_conreq (struct lsformat FAR *, struct lsapformat FAR *,
	struct lsapformat FAR *, unsigned int);
int  LL_datareq (struct lsformat FAR *, mblk_t *);
int  LL_disreq (struct lsformat FAR *);
void L2_disind(struct lsformat FAR *);

	/* x25mbu.c */

void putNbytes (mblk_t *, int,unsigned char *);
void getNbytes (mblk_t *, int, unsigned char *);
void m_freem (mblk_t *);
void mb_join (mblk_t *, mblk_t *);
mblk_t *mb_frag (mblk_t *);
mblk_t *mb_zapdata (mblk_t *);
int  mb_nxtbyte (mblk_t *, bufp_t);
int  mb_getch (struct x25vcformat FAR *, bufp_t);
int  FAR *mtod (mblk_t *);
void mb_gset (struct x25vcformat FAR *, mblk_t *);
int  mb_bulen (mblk_t *);

	/* x25nsuif.c */

void  N_enableinput (int, int);
int  N_CCdetails (int, unsigned char FAR *, struct qosformat FAR *);
int  N_CIdetails (int, unsigned char FAR *, unsigned char FAR *,
	struct qosformat FAR *);
int  N_conrsp (int, int, unsigned char, struct xaddrf FAR *,
	struct qosformat FAR *, mblk_t *);
int  N_conreq (int, int *, unsigned char, struct xaddrf FAR *,
	struct xaddrf FAR *, struct   qosformat FAR *, mblk_t *);
void sh_disc_facilities (int, struct x25vcformat FAR *, struct qosformat FAR *);
int  N_DIdetails (int, unsigned char FAR *, unsigned char FAR *,
	struct xaddrf FAR *, struct qosformat FAR *);
int  N_RIdetails (int, unsigned char FAR *, unsigned char FAR *);
int  N_disreq (int, unsigned char, unsigned char, struct qosformat FAR *,
	struct xaddrf FAR *, struct xaddrf FAR *, mblk_t *);
int  N_rstrsp (int);
int  N_rstreq (int, unsigned char, unsigned char);
int  N_expack (int);
int  N_putexpdata (int, mblk_t *);
int  N_datack (int);
int  N_putdata (int, mblk_t *);

	/* x25pti.c */

int  IDnnPkt (struct lsformat FAR *, unsigned char, unsigned char);
int  ID00Pkt (struct lsformat FAR *, unsigned char, unsigned char);

	/* x25rxin.c */

void Rx_CALL (struct lsformat FAR *, struct x25vcformat FAR *,
	unsigned char, mblk_t *);
void Rx_CAA (struct lsformat FAR *, struct x25vcformat FAR *,
	unsigned char, mblk_t *);
void Rx_RST (struct x25vcformat FAR *, int, mblk_t *);
void Rx_RSC (struct x25vcformat FAR *, int, mblk_t *);
void Rx_RNR (struct x25vcformat FAR *, unsigned char, int);
void Rx_RR (struct x25vcformat FAR *, unsigned char, int);
void Rx_INC (struct x25vcformat FAR *, int, mblk_t *);
void Rx_INT (struct x25vcformat FAR *, int, mblk_t *);
void Rx_CLR (struct lsformat FAR *, struct x25vcformat FAR *, unsigned char,
	int, mblk_t *);
void Rx_CLC (struct lsformat FAR *, struct x25vcformat FAR *, unsigned char,
	int, mblk_t *);
void Rx_DATA (struct x25vcformat FAR *, unsigned char, unsigned char, int,
	mblk_t *);
int  Rx_REG (struct lsformat FAR *, mblk_t *, unsigned char);

	/* x25touser.c */

void DA_touser (struct x25vcformat FAR *);
void EDA_touser (struct x25vcformat FAR *, mblk_t *);
void FL_touser (struct x25vcformat FAR *);
void ED_touser (struct x25vcformat FAR *, mblk_t *);
int  DT_touser (struct x25vcformat FAR *, mblk_t *);
void CC_touser (struct x25vcformat FAR *, mblk_t *);
void CI_touser (struct x25vcformat FAR *, mblk_t *);
int  user_on_NSAP (struct xaddrf FAR *, mblk_t *, int, int FAR *, int FAR *,
	unsigned short);
void reset_confirm_user (struct x25vcformat FAR *, mblk_t *);
int  reset_user (struct x25vcformat FAR *, unsigned char, unsigned char);
int  disc_confirm_user (struct x25vcformat FAR *);
int  redisconnect_user (struct x25vcformat FAR *);
int  disconnect_user (struct x25vcformat FAR *, unsigned char, unsigned char,
	mblk_t *);
void del_sqrl_data (struct x25vcformat FAR *);
void send_sqrl_data (struct x25vcformat FAR *);
void sqrl_PVC_data (struct x25vcformat FAR *, mblk_t *, int);

	/* x25txout */

int  Tx_REG (struct lsformat FAR *, mblk_t *, unsigned char, unsigned char,
	unsigned char, unsigned char);
void Tx_DIAG (struct lsformat FAR *, unsigned char, unsigned char,
	unsigned char, unsigned char, unsigned char);
void confirm_restart (struct lsformat FAR *, mblk_t *);
int  restart_request (struct lsformat FAR *, unsigned char, unsigned char,
	mblk_t *);
int  Tx_RST (struct x25vcformat FAR *, unsigned char, unsigned char);
int  Tx_RSC (struct x25vcformat FAR *);
int  clear_request (struct x25vcformat FAR *);
void maybe_ack (struct x25vcformat FAR *);
int  Tx_EDdata (struct x25vcformat FAR *);
void Tx_Qdata (struct x25vcformat FAR *);
int  short_out (struct x25vcformat FAR *, unsigned char);
void to_link_level (struct x25vcformat FAR *, mblk_t *);
int  handle_x32_registration (struct lsformat FAR *, mblk_t *, unsigned char,
	struct reg_facilities *);
void setup_header(int, mblk_t *);

	/* x25upper.c */

int  close_x25_tracestrm (queue_t  *);
int  ratt_PVC (struct xgldbf *, mblk_t *);
int  att_PVC (struct xgldbf *, mblk_t *);
int  chk_PVC (struct wlcfg *, struct pvcconff FAR *);
struct x25vcformat FAR *PVC_db (uint32, unsigned short);
void rmPVCusr (struct xgldbf *);
void make_pvcdbs (struct wlcfg *);
void pvc_fn_err (struct xgldbf *);
mblk_t *find_connid (int, int *);
mblk_t *scan_ears (int, struct xgldbf *);
void go_frozen (queue_t *, mblk_t *);
void rmCIDI (struct xgldbf *);
void rmXLDAT (queue_t *);
void abort_vc (struct xgldbf *);

	/* x25user.c */

void x_exam_user (struct lsformat FAR *, struct x25vcformat FAR *);
mblk_t *make_c_packet (struct x25vcformat FAR *, unsigned char);
void xusrerror (struct x25vcformat FAR *, unsigned char);

#endif /* _X25_PTYPES_H_ */
