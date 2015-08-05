/* hci_rpg_install_info.h - This header file defines
   functions used to access the install info file on
   the RPG. */

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:23 $
 * $Id: hci_rpg_install_info.h,v 1.7 2009/02/27 22:26:23 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef HCI_RPG_INSTALL_INFO_DEF
#define	HCI_RPG_INSTALL_INFO_DEF

/* Function prototypes. */

int     hci_get_install_info( char *tag, char *buf, int bufsize );
int     hci_set_install_info( char *tag, char *buf );
int	hci_get_install_info_adapt_loaded();
int	hci_set_install_info_adapt_loaded();
int	hci_get_install_info_dev_configured();
int	hci_set_install_info_dev_configured();
int	hci_install_info_set_rpg_host( int argc, char **argv );

#endif
