/*   @(#)x25states.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * x25states.h of snet module
 *
 * SpiderX25
 * @(#)$Id: x25states.h,v 1.1 2000/02/25 17:15:29 john Exp $
 * 
 * SpiderX25 Release 8
 */

/* ----------------------------------------------------------------------
	Two useful defines to differentiate 'in' and 'out' modes
   ---------------------------------------------------------------------- */

#define		X_INWARD	0
#define		X_UTWARD	1
#define		X_IOWARD	2

/* ---- X25 VIRTUAL CIRCUIT STATES ----

   NB:  The ordering of this set is important.
*/

#define		Idle		0	/* Record is not in use         */
#define		AskingNRS	1	/* CR is being validated by NRS */
#define		P1		2	/* VC state is READY            */
#define		P2		3	/* VC in DTE CALL REQUEST       */
#define		P3		4	/* VC in DXE INCOMING CALL      */
#define		P5		5	/* VC in CALL COLLISION         */
#define		DataTransfer	6	/* VC in P4 (see xflags)        */
#define		DXEbusy		7	/* VC in P4, DXE sent RNR	*/
#define		D2		8	/* VC in DTE RESET REQUEST      */
#define		D2pending	9	/* Wanting buffer for RESET     */
#define		WtgRCU		10	/* Waiting U RSC to int.err.    */
#define		WtgRCN		11	/* Waiting X.25 RSC for user    */
#define		WtgRCNpending	12	/* Buffer reqd to enter state   */
#define		P4pending	13	/* Buffer reqd for X.25 RSC     */
#define		pRESUonly	14	/* Buffer for user rst only	*/
#define		RESUonly	15	/* User only being reset	*/
#define		pDTransfer	16	/* Buffer for RSC to user	*/
#define		WRCUpending	17	/* Buffer reqd internal RST     */
#define		DXErpending	18	/* Buffer reqd RST indication   */
#define		DXEresetting	19	/* Waiting U RSC to X.25 RI     */
#define		P6		20	/* VC in DTE CLEAR REQUEST      */
#define		P6pending	21	/* Wanting buffer for CLEAR     */
#define		WUcpending	22	/* Buffer reqd DI no netconn    */
#define		WUNcpending	23	/* Buffer reqd internal DI      */
#define		DXEcpending	24	/* Buffer reqd CLR REQ->User    */
#define		DXEcfpending    25	/* Buffer reqd CLC to User      */
#define		DXEclearing	26	/* Wanting buffer for CLC       */

/* ---- PACKET LEVEL AND LSAP/LINK STATES ---- */

#define		L_connecting	1	/* Connecting to DXE            */
#define		WtgRES		2	/* Connected resolving DXE      */
#define		WtgDXE		3	/* Random wait started          */
#define		L_connected	4	/* Connected and resolved DXE   */
#define		L3restarting	5	/* DTE RESTART REQUEST          */
#define		L_disconnecting	6	/* Waiting link disc reply      */
#define		WtgRpending	7	/* Buffer to enter WtgRES       */
#define		L3rpending	8	/* Buffer to enter L3restarting */
#define		L_dpending	9	/* Buffer to enter L_disconnect */
#define		L_registering   10	/* Registration request         */
