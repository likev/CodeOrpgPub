/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:12:51 $
 * $Id: orpg.h,v 1.79 2014/07/21 20:12:51 ccalvert Exp $
 * $Revision: 1.79 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpg.h

 Description: Global Open Systems Radar Product Generator (ORPG) header
              file.
       Notes: Please place all ORPG_ #defines and macros in orpg_def.h.
 **************************************************************************/


#ifndef ORPG_H
#define ORPG_H


/*
 * System Include Files/Local Include Files
 */

#include <infr.h>              /* Generic Services include file           */

#include <rss_lb.h>            /* Implement RSS LB functions              */
                               /* NOTE: source files should include       */
                               /* rss_isoc.h and or rss_posix1.h as needed*/

#include <orpg_def.h>          /* MUST PRECEDE ALL New RPG HEADER FILES   */

#include <orpgctype.h>         /* Global ORPG C data type include file    */
#include <orpgdat.h>           /* Global ORPG  data include file          */
#include <orpgerr.h>           /* Global ORPG error include file          */
#include <orpgevt.h>           /* Global ORPG event include file          */
#include <orpgcfg.h>           /* Global ORPG config include file         */
#include <orpgstat.h>          /* Global ORPG status include file         */

#include <orpgda.h>            /* liborpg DA module include file          */
#include <orpgdbm.h>           /* liborpg DBM module include file         */
#include <orpgumc.h>           /* liborpg UMC module include file         */
#include <orpgumc_rda.h>       /* liborpg UMC RDA module include file     */
#include <orpgtask.h>          /* liborpg TASK module include file        */
#include <orpgpat.h>           /* liborpg ORPGPAT module include file     */
#include <orpgpgt.h>           /* liborpg ORPGPGT module include file     */
#include <orpgrda.h>           /* liborpg ORPGRDA module include file     */
#include <orpgtat.h>           /* liborpg ORPGTAT module include file     */
#include <orpgvst.h>           /* liborpg ORPGVST module include file     */
#include <orpginfo.h>          /* liborpg ORPGINFO module include file    */
#include <orpgmisc.h>          /* liborpg ORPGMISC module include file    */
#include <orpgalt.h>           /* liborpg ORPGALT module include file     */
#include <orpgbdr.h>           /* liborpg ORPGBDR module include file     */
#include <orpgload.h>          /* liborpg ORPGLOAD module include file    */
#include <orpgdat_api.h>       /* liborpg ORPGDAT module include file     */
#include <orpgcmp.h>           /* liborpg ORPGCMP module include file     */
#include <orpgvcp.h>           /* liborpg ORPGVCP module include file     */
#include <orpgprq.h>           /* liborpg ORPGPRQ module include file     */
#include <orpgrat.h>           /* liborpg ORPGRAT module include file     */
#include <orpgsum.h>           /* liborpg ORPGSUM module include file     */
#include <orpgrda_adpt.h>      /* liborpg ORPGRDA_ADPT module include file*/
#include <orpgccz.h>           /* liborpg ORPGCCZ module include file     */
#include <orpgnbc.h>           /* liborpg ORPGNBC module include file     */
#include <orpgsails.h>         /* liborpg ORPGSAILS module include file   */

/* The following macro defines the current build version number.  It	*
 * is used in the GSM and displayed by the HCI Control/Status task.	*/

#define	RPG_BUILD_VERSION	80

int ORPGCMI_rda_request ();
int ORPGCMI_rda_response ();
int ORPGCMI_request (int line_ind);
int ORPGCMI_response (int line_ind);

#endif /*DO NOT REMOVE!*/
