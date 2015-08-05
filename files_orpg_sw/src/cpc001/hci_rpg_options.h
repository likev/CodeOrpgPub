/*
 * RCS info 
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:24 $
 * $Id: hci_rpg_options.h,v 1.5 2009/02/27 22:26:24 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 * $Log: hci_rpg_options.h,v $
 * Revision 1.5  2009/02/27 22:26:24  ccalvert
 * consolidate HCI code
 *
 * Revision 1.4  2005/10/17 22:58:09  ccalvert
*/

/****************************************************************
		
    Module: hci_rpg_options.h
				
    Description: This is the header file the HCI RPG init options

****************************************************************/

#ifndef HCI_RPG_OPTIONS_H
#define HCI_RPG_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

/*	The following group of modules return information from the	*
 *	RPG initialization options configuration data.			*/

#define	HCI_MAX_INIT_OPTIONS		 32
#define	HCI_INIT_OPTIONS_NAME_LEN	 64
#define	HCI_INIT_OPTIONS_DESC_LEN	512
#define HCI_INIT_OPTIONS_ACTION_LEN	512
#define	HCI_INIT_OPTIONS_STATE_LEN	128
#define	HCI_INIT_OPTIONS_PERM_LEN	  8
#define	HCI_INIT_OPTIONS_GROUP_LEN	 64 
#define	HCI_INIT_OPTIONS_MSG_LEN	128

typedef struct {

	char	*name;		/* The label used for the GUI selection */
	char	*description;	/* A descriptive string for the action	*/
	char	*action;	/* A string containing the command to	*
				 * execute when selected.		*/
	char	*state;		/* A string containing the RPG states	*
				 * in which the action is allowed.	*/
	char	*permission;	/* A string containing the permission	*
				 * required to activate the option.	*/
	char	*group;		/* A string used to organize options by	*
				 * groups.				*/
	char	*msg;		/* A string to be written to the RRG	*
				 * log file when action is invoked.	*/

} Hci_init_options_t;

int	HCI_num_init_options ();
int	HCI_init_options ();
char	*HCI_get_init_options_name (int indx);
char	*HCI_get_init_options_description (int indx);
char	*HCI_get_init_options_action (int indx);
char	*HCI_get_init_options_state (int indx);
char	*HCI_get_init_options_permission (int indx);
char	*HCI_get_init_options_group (int indx);
char	*HCI_get_init_options_msg (int indx);
int 	HCI_init_options_exec(int indx);


#ifdef __cplusplus
}
#endif

#endif

