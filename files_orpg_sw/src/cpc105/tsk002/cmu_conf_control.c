

/***************************************************************************

    cmu_conf_control.c - MPSx00 confuguration and control module.

***************************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2007/02/15 18:40:49 $
 * $Id: cmu_conf_control.c,v 1.15 2007/02/15 18:40:49 jing Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */  


#ifdef DEBUG
static int debug = 1;
#endif

#include <unistd.h>
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include <string.h>
#include <sys/types.h>

#define NOTIFY_STATUS
#define USER_RESTART

#include "cmu_def.h"

#include <debug.h>
#include <nc.h>

#include <mpsproto.h>
#include <uint.h>
#include <sdlpi.h>
#include <ll_proto.h>
#include <x25_proto.h>
#include <ll_control.h>
#include <x25_control.h>
#include <xnetdb.h>

#ifdef LINUX
#include <xstra.h> 
#endif

#define CONFIG_WAITING_TIME 5

typedef struct ll_snioc LL_SNIOC;
typedef struct xll_reg XLL_REG;

static char	*pCtlrNumber;
static NC	*pNc;
static char	*pServer;
static char	*pService;


static int lc_lapb_wan (NC *, char *, char *, char *, char *, char *, char *,
                         char *, Link_struct *link, int class);
static int lc_x25_lapb (NC *pNc, char *pServer, char *pCtlrNumber, 
	char *pX25Label, char *pLapbLabel, char *pMuxLabel, Link_struct *link);
static int Open_DLPI_listener (char *pServer, char *pService);


/*************************************************************************

    Performs MPSx00 X25/PVC configuration.

    Returns DLPI listener fd on success, -1 on non-recoverable failure
    and -2 on recoverable failure.

*************************************************************************/

int CCC_x25_config (int n_links, Link_struct **links) {
    char *pServerLabel;
    char device[4];
    char label[4], x_label[8], l_label[8];
    int i, label_val, dlpifd;
/*    time_t t; */

    pServer     = CC_get_server_name ();
    pService    = "mps";
    pCtlrNumber = "0";
    pNc = 0;		/* not used */

    /* set device specific labels and clean up */
    sprintf (x_label, "XXX%d", links[0]->device);
    sprintf (l_label, "LLL%d", links[0]->device);
/*
    nc_close (pNc, x_label);
    nc_close (pNc, l_label);
*/
/*    nc_delete (pNc, x_label);	 Butler said nc_delete not necessary
    nc_delete (pNc, l_label); */
/*
    t = MISC_systime (NULL);
    while (MISC_systime (NULL) <= t + CONFIG_WAITING_TIME)
	msleep (1000);	*/	/* wait for nc_close to settle down */

    /* open DLPI listener stream */
    if ((dlpifd = Open_DLPI_listener (pServer, pService)) < 0)
	return (-2);

    /* Open x25 layer */
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber,
				"x25", "0", x_label);
    if (pServerLabel == NULL) {
	LE_send_msg (GL_ERROR, "nc_open x25 to %s failed\n", pServer);
	MPSclose (dlpifd);
	return (-2);
    }
    LE_send_msg (LE_VL1, "Opened (x25) %s Answer %s\n", pServer, pServerLabel);

    /* Open lapb layer */
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber, 
				"lapb", "0", l_label);
    if (pServerLabel == NULL) {
        LE_send_msg (GL_ERROR, "nc_open lapb to %s failed\n", pServer);
        return (-1);
    }
    LE_send_msg (LE_VL1, "Opened (lapb) %s Answer '%s'\n", 
						pServer, pServerLabel);

    label_val = 120;
    for (i = 0; i < n_links; i++) {	/* Configure DTE link */
	sprintf (device, "%d", links[i]->port);
	sprintf (label, "%d", label_val++);
	LE_send_msg (LE_VL1, "lc_lapb_wan (DTE) link %s snid=%s label=%s\n", 
					device, links[i]->snid, label);
	if (lc_lapb_wan (pNc, pServer, pService, pCtlrNumber, device, label, 
				l_label, "WWW", links[i], LC_LAPBDTE) < 0)
	    return (-1);
    }

    label_val = 130;
    for (i = 0; i < n_links; i++) {	/* Configure DTE link */
	sprintf (label, "%d", label_val++);
	LE_send_msg (LE_VL1, "lc_x25_lapb (DTE) snid=%s label=%s\n", 
             					links[i]->snid, label);
	if (lc_x25_lapb (pNc, pServer, pCtlrNumber, 
				x_label, l_label, label, links[i]) < 0)
	    return (-1);
	/* disable first time lapb enable after configuration */
	links[i]->lapb_enable_state = ENABLED;
    }

    return (dlpifd);
}

/*************************************************************************

    Links wan layer under lapb. Configures lapb and wan using the lltune
    and wantune utilities.

    Returns 0 on success or -1 on failure.

*************************************************************************/

static int
lc_lapb_wan (NC *pNc, char *pServer, char *pService, char *pCtlrNumber, 
	char *pDevice, char *pLabel, char *pLapbLabel, char *pWanLabel, 
	Link_struct *link, int class) {
	/* pDevice = port number, pLapbLabel = LLL?, 
			pWanLabel = WWW, pLabel = 120 + i */
    int		answer;
    int		lapbMuxid;
    char	command [500];
    char	*pAnswer;
    char	*pServerLabel;
    LL_SNIOC    snioc;

    /* Open wan layer for port pDevice */
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber,
					"wan", pDevice, pWanLabel);
    if (pServerLabel == NULL) {
        LE_send_msg (GL_ERROR, "nc_open wan to %s port %s failed\n", 
					pServer, pDevice);
        return (-1);
    }
    LE_send_msg (LE_VL1, "  Opened wan on %s port %s Answer %s\n", 
					pServer, pDevice, pServerLabel);

    /* Link wan under lapb */
    LE_send_msg (LE_VL1, "  Linking %s under %s for %s\n", 
					pWanLabel, pLapbLabel, pLabel);
    pAnswer = nc_link (pNc, pLapbLabel, pWanLabel, pLabel);
    if (pAnswer == NULL) {
        LE_send_msg (GL_ERROR, "nc_link (%s %s %s) failed\n", 
					pLapbLabel, pWanLabel, pLabel);
        return (-1);
    }

    /* Get code for label pLabel */
    lapbMuxid = nc_muxval (pNc, pLabel);
    if (lapbMuxid == -1) {
        LE_send_msg (GL_ERROR, "nc_muxval failed on %s\n", pLabel);
        return (-1);
    }
    LE_send_msg (LE_VL1, "  Get label value: Label %s value: %d\n", 
						pLabel, lapbMuxid);

    /*
    * Assign the subnet id to the stream. The ioctl goes to the lapb layer.
    * The lapb layer sends a message to the wan layer which allows the wan
    * layer to associate the subnet id with the stream. Later, when
    * wantune executes for this subnetid, the wan layer uses the provided
    * subnet id to locate the stream that the configuration parameters
    * are intended for.
    */
    memset (&snioc, 0, sizeof (snioc));
    snioc.lli_type      = LI_SNID;
    snioc.lli_snid      = INT_BSWAP_L (snidtox25 (link->snid));
    snioc.lli_index     = INT_BSWAP_L (lapbMuxid);
    snioc.lli_class     = class;	/* LC_LAPBDTE or LC_LAPBDCE */

    answer = nc_strioc (pNc, pLabel, L_SETSNID, 10, (unsigned char *)&snioc,
            sizeof (snioc));
    if (answer == -1) {                 /* failed */
        LE_send_msg (GL_ERROR, "Error on nc_strioc L_SETSNID %s\n", pLabel);
        return (-1);
    }
    LE_send_msg (LE_VL1, "  Sent L_SETSNID to %s\n", pLabel);

    /* Tune wan and lapb */
    sprintf (command, "wantune -P -d %s -s %s -t %s -c %s %s",
	    pDevice, link->snid, pServer, pCtlrNumber, link->wan_conf);

    LE_send_msg (LE_VL1, "  Issue %s\n", command);
    answer = nc_shell (pNc, command);
    if (answer == 0) {
        LE_send_msg (GL_ERROR, "Error on nc_shell wantune\n");
        return (-1);
    }

    sprintf (command, "lltune -P -p lapb -s %s -d 0 -t %s -c %s %s",
	    link->snid, pServer, pCtlrNumber, link->lapb_conf);

    LE_send_msg (LE_VL1, "  Issue %s\n", command);
    answer = nc_shell (pNc, command);
    if (answer == 0) {
        LE_send_msg (GL_ERROR, "Error on nc_shell lltune\n");
        return (-1);
    }
    return (0);
}

/*************************************************************************

    Links lapb under x25 and configures x25 using x25tune utility. Then
    assigns subnet ID. Assigning the subnet may or may not cue the x25
    network software to try to bring up the lapb (link) layer depending
    on the wan configuration. See WANTEMPLATE and WAN_auto_enable.

    Returns 0 on success or -1 on failure.

*************************************************************************/

static int
lc_x25_lapb (NC *pNc, char *pServer, char *pCtlrNumber, char *pX25Label, 
	char *pLapbLabel, char *pMuxLabel, Link_struct *link) {
	/* pX25Label = XXX0, pLapbLabel = LLL?, pMuxLabel = 130 + i, 
            links[i]->snid */
    int	answer;
    char command[500];
    int	x25Muxid;
    char *pAnswer;
    XLL_REG xlreg;

    /* Link lapb under x25 */
    LE_send_msg (LE_VL1, "  Link %s under %s for %s\n", 
				pLapbLabel, pX25Label, pMuxLabel);

    pAnswer = nc_link (pNc, pX25Label, pLapbLabel, pMuxLabel);
    if (pAnswer == NULL) {
        LE_send_msg (GL_ERROR, "nc_link (%s %s %s) failed\n", 
				pX25Label, pLapbLabel, pMuxLabel);
        return (-1);
    }

    /* Get code for label muxid label */
    x25Muxid = nc_muxval (pNc, pMuxLabel);
    if (x25Muxid == -1) {
        LE_send_msg (GL_ERROR, "nc_muxval failed on %s\n", pMuxLabel);
        return (-1);
    }
    LE_send_msg (LE_VL1, "  Get label value: Label %s value %d\n", 
						pMuxLabel, x25Muxid);

    /* Tune x25 for subnet */
    sprintf (command, "x25tune -P -s %s -d 0 -t %s -c %s %s",
			link->snid, pServer, pCtlrNumber, link->x25_conf);

    LE_send_msg (LE_VL1, "  Issue %s\n", command );

    answer = nc_shell (pNc, command);
    if (answer == 0) {
        LE_send_msg (GL_ERROR, "Error on nc_shell x25tune\n");
        return (-1);
    }

    /*
    * Assign subnet. Assigning the subnet may or may not cue the x25
    * network software to try to bring up the lapb (link) layer depending
    * on the wan configuration. See WANTEMPLATE and WAN_auto_enable.
    */
    LE_send_msg (LE_VL1, "  Issue N_snident ioctl to %s\n", pMuxLabel);
    memset (&xlreg, 0, sizeof (xlreg));
    xlreg.snid          = INT_BSWAP_L (snidtox25 (link->snid));
    xlreg.dl_sap        = 0;
    xlreg.lmuxid        = INT_BSWAP_L (x25Muxid);
    xlreg.dl_max_conind = SHORT_BSWAP_L (1);
    answer = nc_strioc (pNc, pMuxLabel, N_snident, 10,
			(unsigned char *)&xlreg, sizeof (xlreg));
    if (answer == -1) {                 /* failed */
        LE_send_msg (GL_ERROR, 
			"Error on nc_strioc N_snident snid %s, %d, %s\n", 
				link->snid, x25Muxid, pMuxLabel);
        return (-1);
    }
    return (0);
}

/*************************************************************************

    Enables/disables (command = W_ENABLE/W_DISABLE) the port 
    "pSnid" ("A", "B", "C" ...).

    Returns 0 on success or -1 on failure.

*************************************************************************/

#include <wan_control.h>

int CCC_wanCommand (char *pSnid, unsigned int command) {
    int                 answer;
    char                *pServerLabel;
    char		wanLabel [100];
    struct wan_hdioc    wan_tn;
    NC			*pNc;

    pNc = NULL;			/* not used */

    /* Open wan layer */
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber, 
					"wan", "0", "WWWX");
    if (pServerLabel == NULL) {
        LE_send_msg (GL_ERROR, 
			"nc_open wan to %s device 0 failed.\n", pServer);
        return (-1);
    }
    LE_send_msg (LE_VL3, 
		"    Opened wan on %s Answer %s\n", pServer, pServerLabel);
    strcpy (wanLabel, pServerLabel);

    /* Build and send command to wan */
    if (command == W_DISABLE)
	LE_send_msg (LE_VL3, "    disabling wan layer of subnet %s\n", pSnid);
    else
	LE_send_msg (LE_VL3, "    enabling wan layer of subnet %s\n", pSnid);
    memset (&wan_tn, 0, sizeof (wan_tn));
    wan_tn.w_type       = WAN_PLAIN;
    wan_tn.w_snid       = INT_BSWAP_L (snidtox25 (pSnid));
    answer = nc_strioc (pNc, wanLabel, command, 10,
            (unsigned char *) &wan_tn, sizeof (wan_tn));
    if (answer == -1) {                 /* failed */
        LE_send_msg (GL_ERROR, "Error on nc_strioc disable/enable wan\n");
        return (-1);
    }

    answer = nc_close (pNc, wanLabel);
    if (answer == 0) {
        LE_send_msg (GL_ERROR, "Error closing %s\n", wanLabel);
        return (-1);
    }

    if (command == W_ENABLE)
	LE_send_msg (LE_VL3, "    enabling subnet %s\n", pSnid);

    return (0);
}


/*************************************************************************

    Enables/disables (command = L_LINKENABLE/L_LINKDISABLE) the port 
    "pSnid" ("A", "B", "C" ...).

    Returns 0 on success or -1 on failure.

*************************************************************************/

int CCC_lapbCommand (char *pSnid, unsigned int command) {
    int                 answer;
    char                *pServerLabel;
    char		lapbLabel [100];
/*    struct ll_sntioc    ll_tn;		 the new struct */
    struct ll_hdioc     ll_tn; /* The old struct - should work too */
    NC			*pNc;

    pNc = NULL;			/* not used */

    /* Open lapb layer */
    pServerLabel = nc_open (pNc, pServer, pService, pCtlrNumber, 
						"lapb", "0", "LLLX");
    if (pServerLabel == NULL) {
        LE_send_msg (GL_ERROR, 
			"nc_open lapb to %s device 0 failed.\n", pServer);
        return (-1);
    }
    LE_send_msg (LE_VL3, "    Opened lapb on %s Answer %s\n", 
					pServer, pServerLabel);
    strcpy (lapbLabel, pServerLabel);

    /* Build and send command to lapb */
    if (command == L_LINKDISABLE)
	LE_send_msg (LE_VL3, "    disabling lapb layer of subnet %s\n", pSnid);
    else
	LE_send_msg (LE_VL3, "    enabling lapb layer of subnet %s\n", pSnid);
    memset (&ll_tn, 0, sizeof (ll_tn));
    ll_tn.lli_snid       = INT_BSWAP_L (snidtox25 (pSnid));
    answer = nc_strioc (pNc, lapbLabel, command, 10,
            (unsigned char *) &ll_tn, sizeof (ll_tn));
    if (answer == -1) {                  /* If failed */
        LE_send_msg (GL_ERROR, "Error on nc_strioc disable/enable lapb\n");
        return (-1);
    }
    answer = nc_close (pNc, lapbLabel);
    if (answer == 0) {
        LE_send_msg (GL_ERROR, "Error nc_close lapb %s\n", lapbLabel);
        return (-1);
    }

    return (0);
}

/*************************************************************************

    Opens the DLPI listener stream.

    Returns the fd on success or -1 on failure.

*************************************************************************/

static int Open_DLPI_listener (char *pServer, char *pService) {
    OpenRequest		oreqX25;
    int			x25Fd;
    char		*pProto;
    struct xstrioctl	strioctl;

    pProto = "x25";

    memset (&oreqX25, 0, sizeof (oreqX25));
    oreqX25.dev 	= 0;
    oreqX25.port 	= 0;
    oreqX25.ctlrNumber 	= 0;
    oreqX25.flags	= CLONEOPEN;
    strcpy (oreqX25.serverName, pServer);
    strcpy (oreqX25.serviceName, pService);
    strcpy (oreqX25.protoName, pProto);

    if ((x25Fd = MPSopen (&oreqX25)) < 0) {
	LE_send_msg (GL_ERROR, 
			"MPSopen DLPI listener to %s failed", pServer);
	return (-1);
    }

    strioctl.ic_cmd = N_DLPInotify;
    strioctl.ic_timout = -1;
    strioctl.ic_len = 0;
    strioctl.ic_dp = NULL;

    if (MPSioctl (x25Fd, X_STR, &strioctl) < 0) {
	LE_send_msg (GL_ERROR, 
			"MPSioctl N_DLPInotify failed (errno %d)", errno);
	return (-1);
    }

    LE_send_msg (LE_VL1, "DLPI listener done");
    return (x25Fd);
}

/*************************************************************************

    Restart control of the stream.

    Returns 0 on success or -1 on failure.

*************************************************************************/

int CCC_control_restart (int func, char *pSnid) {
    typedef struct {
	uint32 snid;
	char   enabled;
    } ioctldata;
    static int x25Fd = -1;
    ioctldata data;
    struct xstrioctl strioctl;

    if (x25Fd < 0) {
        OpenRequest oreqX25;

	memset (&oreqX25, 0, sizeof (oreqX25));
	oreqX25.dev 	= 0;
	oreqX25.port 	= 0;
	oreqX25.ctlrNumber 	= 0;
	oreqX25.flags	= CLONEOPEN;
	strcpy (oreqX25.serverName, pServer);
	strcpy (oreqX25.serviceName, pService);
	strcpy (oreqX25.protoName, "x25");
	if ((x25Fd = MPSopen (&oreqX25)) < 0) {
	    LE_send_msg (GL_ERROR, 
			"MPSopen control_restart to %s failed (errno %d)", 
						pServer, MPSerrno);
	    return (-1);
	}
    }

    data.snid = INT_BSWAP_L (snidtox25 (pSnid));

    switch (func) {

	case CCC_SEND_RESTART:
	    strioctl.ic_cmd = N_sendRESTART;
	    strioctl.ic_len = 4;
	    strioctl.ic_dp = (char *)&data;
	    break;

	case CCC_ENABLE_TDX:
	    strioctl.ic_cmd = N_tempDXFER;
	    strioctl.ic_len = 5;
	    strioctl.ic_dp = (char *)&data;
	    data.enabled = 1;
	    break;

	case CCC_DISABLE_TDX:
	    strioctl.ic_cmd = N_tempDXFER;
	    strioctl.ic_len = 5;
	    strioctl.ic_dp = (char *)&data;
	    data.enabled = 0;
	    break;

	default:
	    LE_send_msg (GL_ERROR, 
			"CCC_control_restart errro: unexpected func %d", func);
/*	    MPSclose (x25Fd); */
	    return (-1);
	    break;
    }

    strioctl.ic_timout = -1;

    if (MPSioctl (x25Fd, X_STR, &strioctl) == MPS_ERROR) {
	LE_send_msg (GL_ERROR, 
	    "MPSioctl control_restart (func %d, subnet %s) failed (errno %d)", 
						func, pSnid, MPSerrno);
/*	MPSclose (x25Fd); */
	return (-1);
    }

/*    MPSclose (x25Fd); */
    return (0);
}







