/****************************************************************
		
	File: get_client.c	

	Purpose: This module contains client connection handshaking 
		functions for the RMT server. 

*****************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2013/03/07 20:31:37 $
 * $Id: rmt_get_client.c,v 1.25 2013/03/07 20:31:37 jing Exp $
 * $Revision: 1.25 $
 * $State: Exp $
 */  


/*** System include files ***/

#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>


#include <rmt.h>
#include <net.h>
#include <misc.h>
#include <en.h>
#include "rmt_def.h"


/*** Definitions / macros / types ***/

#define WILD_ADDR 0

struct client_record {          /* configured clients */
    char *name;
    unsigned int addr;		/* LBO. If WILD_ADDR, the 
				   permission contains a * in one of the 4 
				   fields. */
    int disconnected;		/* 0: connected; 1: disconnected; 
				   -1: unknown */
};

#define PW_FAILED  -3		/* return value of Receive_check_password */
#define PW_NOT_READY -4		/* return value of Receive_check_password */
#define PW_INVALID -5		/* return value of Receive_check_password */

#define MAX_AUTH_FAILURES 3	/* maximum number of auth failed pending 
				   clients for a single host */
#define AUTH_TIMER     20	/* maximum authen. time (seconds) for a 
				   client */
#define BAD_CL_LOCK_TIME 2	/* time period for locking a failed host */

enum {PCL_PENDING, PCL_OK};	/* values for Pending_client_t.state */

typedef struct {         		/* pending client srtucture */
    int fd;				/* client fd */
    unsigned int addr;			/* client IP address, LBO */
    char pw_encrypt[ENCRYPT_LEN];	/* encripted password */
    int enc_len;			/* length of encripted password */
    time_t time;			/* connection time */
    int msg_in;				/* bytes received */
    short state;			/* pending state */
    short pw_needed;			/* boolean: password is required */
    char msg[AUTH_MSG_LEN];		/* received message */
} Pending_client_t;

static void *Pcl_tblid = NULL;		/* pending RPC client table id */
static Pending_client_t *Pcls;		/* pending client table */
static int N_pcls = 0;			/* number of pending clients */

static void *Cl_tblid = NULL;		/* configured client table id */
static struct client_record *Client_list;
					/* list of configured client hosts */
static int Client_cnt = 0;		/* Number of items in Client_list */

static int Send_auth_msg (int, int, char *);
static void Input_poll_update ();
static int Send_security_message (Pending_client_t *pcl);
static int Recv_auth_msg (Pending_client_t *pcl);
static int Receive_check_password (Pending_client_t *pcl);
static int Check_host_permission (unsigned int cid);
static struct client_record *Add_client (char *name, unsigned int ip);


/***********************************************************************

    Description: This function accepts a client connection request and 
		starts the handshaking process if a client is connected. 
		It, then, checks if any pending client handshaking is 
		completed. 

    Input:	fd - The RPC parent fd.

    Outputs:	cl_type - type of the new client.
		cl_pid - ID of the new client.
		cl_addr - IP address of the new client (LBO).

    Return:	This function returns the handshaking completed client 
		socket fd if there is any. It returns FAILURE otherwise.

***********************************************************************/

int GCLD_get_client (int fd, int *cl_type, 
				int *cl_pid, unsigned int *cl_addr) 
{
    int sockfd;
    unsigned long cid;	/* LBO */
    int i;

    while (Pcl_tblid == NULL) {
	Pcl_tblid = MISC_open_table (
		sizeof (Pending_client_t), 8, 0, &N_pcls, (char **)&Pcls);
	if (Pcl_tblid == NULL)
	    msleep (100);
    }

    /* see whether any client is ready */
    for (i = 0; i < N_pcls; i++) {
	if (Pcls[i].state == PCL_OK) {
	    int clfd;
	    Auth_msg_t *a;

	    clfd = Pcls[i].fd;
	    a = (Auth_msg_t *)Pcls[i].msg;
	    *cl_type = a->type;
	    *cl_pid = a->pid;
	    *cl_addr = Pcls[i].addr;
	    MISC_table_free_entry (Pcl_tblid, i);
	    Input_poll_update ();
	    return (clfd);
	}
    }

    sockfd = SOCD_accept_connection (fd, &cid);	/* try to accept */
    if (sockfd >= 0) {			/* process a new connection */
	int new_ind, c_cnt;
	Pending_client_t *new_pcl;

	/* check host id (is permission granted for the host?) */
	if (Check_host_permission (cid) == FAILURE) {
	    close (sockfd);
	    return (FAILURE);
	}

	/* make sure there are not too many auth failed pending client from 
	   the remote host */
	c_cnt = 0;
	for (i = 0; i < N_pcls; i++) {
	    if (Pcls[i].addr == cid && Pcls[i].fd < 0)
		c_cnt++;
	}

	if (c_cnt > MAX_AUTH_FAILURES) {
	    char buf[64];
	    Send_auth_msg (sockfd, RET_MSG_SIZE, "Rejected_auth");
	    MISC_log ("Too many pending auth failed clients from %s", 
				NET_string_IP (cid, 0, buf));
	    close (sockfd);
	    return (FAILURE);
	}

	/* register as a pending client */
	while ((new_pcl = (Pending_client_t *)MISC_table_new_entry 
					(Pcl_tblid, &new_ind)) == NULL)
	    msleep (100);
	new_pcl->addr = cid;
	new_pcl->time = MISC_systime (NULL);
	new_pcl->fd = sockfd;
	new_pcl->msg_in = 0;
	new_pcl->state = PCL_PENDING;
	if (strlen (SECD_get_password ()) > 0)
	    new_pcl->pw_needed = 1;
	else
	    new_pcl->pw_needed = 0;

	/* register for input polling */
	Input_poll_update ();

	/* send password request */
	if (Send_security_message (new_pcl) == FAILURE) {
	    MISC_table_free_entry (Pcl_tblid, new_ind);
	    Input_poll_update ();
	    close (sockfd);
        }
    }
    return (FAILURE);
}

/***********************************************************************

    Description: This function is called when a pending RPC client 
		socket "fd" is ready for read.

    Input:	fd - The client fd ready for read. If fd < 0, this is
		called for housekeeping.

    Return:	Non_zero if the client handshaking is completed or
		zero otherwise.

***********************************************************************/

int GCLD_client_sock_ready (int fd)
{
    static time_t last_time = 0;
    time_t cr_time;   /* current time */
    int i;

    if (N_pcls == 0)
	return (0);

    cr_time = MISC_systime (NULL);
    if (last_time != cr_time) {		/* time out old clients */
	for (i = 0; i < N_pcls; i++) {
	    if (cr_time > AUTH_TIMER + Pcls[i].time) {
		if (Pcls[i].fd >= 0)
		    close (Pcls[i].fd);
		MISC_table_free_entry (Pcl_tblid, i);	/* free the record */
		i--;
		Input_poll_update ();
	    }
	}
	last_time = cr_time;
    }

    if (fd >= 0) {
	Pending_client_t *cl;
	int ret;

	for (i = 0; i < N_pcls; i++)
	    if (Pcls[i].fd == fd)
		break;
	if (i >= N_pcls)
	    return (0);

	cl = Pcls + i;
	ret = Receive_check_password (cl);

	if (ret == SUCCESS) {           /* authen. success */
	    cl->state = PCL_OK;
	    return (1);
	}
	else if (ret == PW_INVALID) {	/* bad client */
	    close (cl->fd);
	    cl->time = cr_time - AUTH_TIMER + BAD_CL_LOCK_TIME; 
	    cl->fd = -1;		/* lock the record */
	}
	else if (ret == PW_FAILED) {	/* connection lost */
	    close (cl->fd);
	    MISC_table_free_entry (Pcl_tblid, i);
	    Input_poll_update ();
	}
    }

    return (0);
}

/***********************************************************************

    Description: This function sends the pending client fds for input
		monitoring.

***********************************************************************/

static void Input_poll_update ()
{
    static int buf_size = 0;
    static int *buf = NULL;
    int i;

    if (N_pcls > buf_size) {		/* realloc the buffer */
	int new_size, *pt;

	new_size = N_pcls + 32;
	pt = (int *)MISC_malloc (new_size * sizeof (int));
	buf_size = new_size;
	if (buf != NULL)
	    MISC_free (buf);
	buf = pt;
    }

    for (i = 0; i < N_pcls; i++)
	buf[i] = Pcls[i].fd;
    MSGD_register_rpc_waiting_fds (N_pcls, buf);

    return;
}

/****************************************************************
			
	This function sends an authentication request to a client. 
	If a password is not required (the "Password"
	string is empty), the function sends a "No_password" message 
	to the client. If a password is required (Password string 
	is non-empty), the function sends a "Key:(8 byte key)" 
	message to the client. The randomly generated key will be 
	used by the client for encrypting its password.
	The encrypt password is recorded for later comparison.

    Input:	pcl - pending client struct. 

    Return:	SUCCESS or FAILURE if sending the message failed.

****************************************************************/

static int Send_security_message (Pending_client_t *pcl)
{
    char *msg;

    if (pcl->pw_needed) {
	char msg_buf[RET_MSG_SIZE];	/* message for sending out */
	char pswd_buf[PASSWORD_SIZE + KEY_SIZE];  /* key + password */
	int enc_len;                /* length of encrypted password */
	int key[KEY_SIZE/4 + 1];   /* random key */
	int len;

	/* ask for password */
	SECD_get_random_key ((char *)key, KEY_SIZE);

	/* the message for client */
	len = KEY_SIZE;
	if (len + 4 > RET_MSG_SIZE) len = RET_MSG_SIZE - 4;
	strncpy (msg_buf, "Key:", 4);
	memcpy (msg_buf + 4, (char *)key, len);
	msg = msg_buf;

	/* encode the password for later checking */
	memcpy (pswd_buf, (char *)key, KEY_SIZE);
	strcpy (pswd_buf + KEY_SIZE, SECD_get_password ());   /*  */
	enc_len = ENCR_encrypt (pswd_buf, ENCRYPT_LEN, pcl->pw_encrypt);
	pcl->enc_len = enc_len;
    }
    else
	msg = "No_password";

    if (Send_auth_msg (pcl->fd, RET_MSG_SIZE, msg) == FAILURE) {
	char buf[64];
	MISC_log ("Failed sending sec msg: %s", 
				NET_string_IP (pcl->addr, 0, buf));
	return (FAILURE);
    }

    return (SUCCESS);
}

/****************************************************************
			
	This function receives an authentication message from the 
	client "pcl". The message contains the user type, ID and
	password. If password is required, the encrypted password 
	is then compared with the local encrypted password. If the 
	password check is OK, the function returns a "Password_OK" 
	message. If the client's password is invalid, this function 
	sends a "Pw_INVALID" message to the client. 

     Input:	pcl - the pending client struct.

     Return:	PW_NOT_READY - message is not ready.
		PW_FAILED - fd is bad or disconnected.
		PW_INVALID - password authentication failed.
		SUCCESS - success.

****************************************************************/

static int Receive_check_password (Pending_client_t *pcl)
{
    int ret;
    Auth_msg_t *a;

    /* receive msg */
    ret = Recv_auth_msg (pcl);
    if (ret == FAILURE)
	return (PW_FAILED);

    if (pcl->msg_in < AUTH_MSG_LEN)
	return (PW_NOT_READY);

    a = (Auth_msg_t *)pcl->msg;
    a->pid = ntohl (a->pid);
    a->type = ntohl (a->type);

    if (pcl->pw_needed) {
	char *pw;
	char msg[RET_MSG_SIZE];	/* message for sending out */
	int i;

	pw = a->password;
	for (i = 0; i < pcl->enc_len; i++) {	/* compare */
	    if (pw[i] != pcl->pw_encrypt[i]) {
		char buf[64];
		MISC_log ("Password check fail: %s", 
				NET_string_IP (pcl->addr, 0, buf));
		strcpy (msg, "Pw_INVALID");
		Send_auth_msg (pcl->fd, RET_MSG_SIZE, msg);
		return (PW_INVALID);
	    }
	}

	strcpy (msg, "Password_OK");
	if (Send_auth_msg (pcl->fd, RET_MSG_SIZE, msg) == FAILURE)
	   return (PW_FAILED);
    }

    return (SUCCESS);
}

/****************************************************************

	This function reads data from the pending client "pcl".

    Return:	FAILURE if the socket is disconnected or a fatal
		error is encountered. SUCCESS otherwise.

****************************************************************/

static int Recv_auth_msg (Pending_client_t *pcl)
{
    int ret;			/* number of bytes read */

    ret = read (pcl->fd, pcl->msg + pcl->msg_in, 
				AUTH_MSG_LEN - pcl->msg_in);

    if (ret == 0)
	return (FAILURE);	/* socket disconnected */

    if (ret < 0 && errno != EWOULDBLOCK)
	return (FAILURE);

    pcl->msg_in += ret;
    return (SUCCESS);
}

/****************************************************************
			
	GCLD_initialize_host_table()		Date: 2/23/94

	This function reads in all listed host names from
	the RMT server configuration file. 

	The file is a ASCII file containing lines. Each host is 
	specified in the file with a line read as

		Client: host_name
		Client: 125.23.45.6

	Leading spaces and TABs are ignored. Every host name read is
	echoed on the screen for verification. The host_name is converted
	to the internet address and stored for later comparison.

	If the RMT configuration file is not found or the file does 
	not contain any "Client:" item, the function assumes a failure 
	and returns FAILURE.

	This function allocates space for the host names. It will
	return FAILURE if the malloc calls fail.

	It closes the file and returns number of configured clients on success.

	Only the first NAME_SIZE characters in a line are guaranteed to
	be read. Remaining characters are truncated and discarded.
*/

int
GCLD_initialize_host_table 
(
    char *conf_name
)
{
    FILE *fl;				/* file handle */
    char name[NAME_SIZE];
    char line[NAME_SIZE];
    int n_ladds, i, use_list_order, changed;
    unsigned int *ladds, ip, lip, rmtport_ip;

    /* open the conf file */
    fl = MISC_fopen (conf_name, "r");
    if (fl == NULL) {
	MISC_log ("Can not open conf file %s\n", conf_name);
	return (FAILURE);
    }

    /* create the registered client table */
    if (Cl_tblid != NULL) {
	for (i = 0; i < Client_cnt; i++)
	    MISC_free (Client_list[i].name);
	MISC_free_table (Cl_tblid);
    }
    Cl_tblid = MISC_open_table (sizeof (struct client_record), 
			    16, 0, &Client_cnt, (char **)&Client_list);

    n_ladds = NET_find_local_ip_address (&ladds);
    if (n_ladds <= 0) {
	MISC_log ("Can not find local network interface\n");
	return (FAILURE);
    }
    rmtport_ip = PNUM_get_local_ip_from_rmtport ();

    /* reads in the file */
    use_list_order = 0;
    Client_cnt = 0;
    lip = INADDR_NONE;
    while (fgets (line, NAME_SIZE, fl) != NULL) {
	char *p, *key, *key1;
	p = line;
	while (*p == ' ' || *p == '\t')
	    p++;

	key = "Disconnect timer:";
	key1 = "Use listed order";
	if (strncmp (p, key, strlen (key)) == 0) {
	    int t;
	    if (sscanf (p + strlen (key), "%d", &t) != 1 ||
		t <= 0)
		MISC_log ("Unexpected Disconnect timer spec - Ignored\n");
	    else {
		MISC_log ("Disconnect timer set to %d\n", t);
		if (t > 0) {
		    EN_disconnect_time ((unsigned int)t);
		    PNUM_disconnect_time (t); 
		}
	    }
	}

	else if (strncmp (p, key1, strlen (key1)) == 0) {
	    use_list_order = 1;
	    MISC_log ("Use host list instead of IP for ordering\n");
	}

	else if (strncmp (p, "Client:", 6) == 0) {
	    int len, wild;
	    struct client_record *new_cl;
	    char *cpt, buf[64];

	    sscanf (p + 7, "%s", name);
	    len = strlen (name);
	    if (len == 0)
		continue;

	    for (i = 0; i < Client_cnt; i++) {
		if (strcmp (Client_list[i].name, name) == 0)
		    break;
	    }
	    if (i < Client_cnt) {
		MISC_log ("Duplicated client name %s\n", name);
		return (FAILURE);
	    }

            /* address is stored in local byte order */
	    cpt = name;
	    wild = 0;
	    while (*cpt != '\0') {
		if (*cpt == '*') {
		    *cpt = '0';
		    wild = 1;
		}
		cpt++;
	    }

	    ip = WILD_ADDR;
	    if (!wild) {
		ip = NET_get_ip_by_name (name);
		if (ntohl (ip) == INADDR_NONE) {
		    MISC_log ("Bad host name: %s\n", name);
		    return (FAILURE);
		}
		for (i = 0; i < Client_cnt; i++) {
		    if (Client_list[i].addr == ntohl (ip))
			break;
		}
		if (i < Client_cnt) {
		    MISC_log ("Duplicated client IP (%s)\n", name);
		    return (FAILURE);
		}
		for (i = 0; i < n_ladds; i++) {
		    if (ladds[i] == ip) {
			if (rmtport_ip == INADDR_NONE) {
			    if (lip == INADDR_NONE) {
				lip = ip;
				MISC_log ("%s selected as local host\n", name);
			    }
			    else {
				MISC_log ("Too many local host (%s)\n", name);
				return (FAILURE);
			    }
			}
			else {
			    if (ip == rmtport_ip) {
				lip = ip;
				MISC_log ("%s selected as local host\n", name);
			    }
			}
		    }
		}
	    }

	    if ((new_cl = Add_client (name, ip)) == NULL)
		return (FAILURE);

	    if (wild) {
		new_cl->addr = WILD_ADDR;
		MISC_log ("Client: %s\n", name);
	    }
	    else
		MISC_log ("Client: %s (IP %s)\n", 
				name, NET_string_IP (ip, 1, buf));
	}
    }
    MISC_fclose (fl);

    if (Client_cnt == 0) {
	MISC_log ("No client host found in config file %s\n", conf_name);
	return (FAILURE);
    }
    if (lip == INADDR_NONE) {
	MISC_log ("No local IP found in config file %s\n", conf_name);
	MISC_log ("-->Try using \"ifconfig -a\" to get local IP\n");
	return (FAILURE);
    }

    /* sort the client list */
    changed = 1;
    while (changed) {
	changed = 0;
	for (i = 1; i < Client_cnt; i++) {
	    if (Client_list[i].addr == WILD_ADDR)
		continue;
	    if (Client_list[i - 1].addr == WILD_ADDR || 
		(!use_list_order && 
			Client_list[i].addr < Client_list[i - 1].addr)) {
		struct client_record z = Client_list[i];
		Client_list[i] = Client_list[i - 1];
		Client_list[i - 1] = z;
		changed = 1;
	    }
	}
    }

    {		/* save the host names for host index look up */
	int cnt, len, size, nc;
	unsigned int *addr, *lix;
	char *buf, *p;
	cnt = nc = 0;
	for (i = 0; i < Client_cnt; i++) {
	    if (Client_list[i].addr == WILD_ADDR)
		break;
	    cnt += strlen (Client_list[i].name) + 1;
	    nc++;
	}
	size = cnt + sizeof (int) * (nc + 2);
	buf = MISC_malloc (size);
	addr = (unsigned int *)buf;
	addr[0] = nc;
	lix = addr + 1;
	addr += 2;
	p = (char *)(addr + nc);
	*lix = 0;
	for (i = 0; i < nc; i++) {
	    len = strlen (Client_list[i].name);
	    memcpy (p, Client_list[i].name, len + 1);
	    p += len + 1;
	    addr[i] = Client_list[i].addr;
	    if (addr[i] == ntohl (lip))
		*lix = i + 1;
	}
	RMT_lookup_host_index (RMT_LHI_SET, buf, size);	/* for server use */
	if (PNUM_access_hosts_file (1, buf, size) < 0) {  /* save in file */
	    free (buf);
	    return (FAILURE);
	}
	free (buf);
    }

    return (Client_cnt);
}

/*************************************************************************

    Adds client of "name" and "ip" to the client table.

*************************************************************************/

struct client_record *Add_client (char *name, unsigned int ip) {
    struct client_record *new_cl;
    int ind;

    new_cl = (struct client_record *)MISC_table_new_entry 
						(Cl_tblid, &ind);
    if (new_cl == NULL) {
	MISC_log ("Malloc error\n");
	return (NULL);
    }
    new_cl->name = (char *)MISC_malloc (strlen (name) + 1);
    strcpy (new_cl->name, name);
    new_cl->disconnected = -1;
    new_cl->addr = ntohl (ip);
    return (new_cl);
}
/********************************************************************

    Performs host index/host name lookup. The host index starts from 1
    so the LB notification registration will work. If func =
    RMT_LHI_H2IX, "buf" is a pointer to a host name and the function
    returns the host index. If func = RMT_LHI_I2IX, "buf" is a pointer
    to a ip (unsigned int) and the function returns the host index. If
    func = RMT_LHI_IX2H, *buf, where "buf" is a pointer to a char *,
    returns the host name of host index "ind". If func = RMT_LHI_IX2I,
    *buf, where buf is a pointer to a int, returns the host ip of host
    index "ind". For the IX2* functions, index = 0 indicated the local
    host and the success return is the actual index. If func =
    RMT_LHI_SET, the hosts data in buf of "ind" bytes is passed to the
    function. If func = RMT_LHI_LIX, the index of the local host is
    returned. Returns -1 on lookup failure or -2 if the hosts info is
    not available. This function does not reread the hosts file
    assuming that all apps must be restarted if one changes rssd
    configuration. IP io is in NBO.

********************************************************************/

int RMT_lookup_host_index (int func, void *buf, int index) {
    static char *data = NULL;
    static int n_hosts = -1;
    int i;
    unsigned int *addrs, *lix;
    char *names;

    if (func == RMT_LHI_SET) {	/* In server, data is passed in directly */
	if (data != NULL)
	    free (data);
	data = MISC_malloc (index);
	memcpy (data, buf, index);
	n_hosts = *((int *)data);
	return (0);
    }

    if (n_hosts < 0) {	/* In client, data is read from rssd hosts file */
	int s, nb, err;
	s = 256;
	while (1) {
	    if (data != NULL)
		free (data);
	    data = MISC_malloc (s);
	    nb = PNUM_access_hosts_file (0, data, s);
	    if (nb < 0)
		return (-2);
	    if (nb < s) {
		data[nb] = '\0';
		break;
	    }
	    s *= 2;
	}
	err = 1;		/* verify the data */
	if (nb >= sizeof (int)) {
	    char *p;
	    int n, cnt;
	    n = *((int *)data);
	    if (nb >= (n + 2) * sizeof (int) + n) {
		p = data + (n + 2) * sizeof (int);
		cnt = 0;
		while (p < data + nb) {
		    p += strlen (p) + 1;
		    cnt++;
		}
		if (cnt == n) {
		    err = 0;
		    n_hosts = cnt;
		}
	    }
	}
	if (err) {
	    MISC_log ("RMT: BAD rssd hosts data (%d bytes)\n", nb);
	    return (-2);
	}
    }

    lix = (unsigned int *)(data + sizeof (int));
    addrs = lix + 1;
    names = (char *)(addrs + n_hosts);
    if (func == RMT_LHI_H2IX) {  		/* host to index */
	char *name, *p;
	name = (char *)buf;
	p = names;
	for (i = 0; i < n_hosts; i++) {
	    if (strcmp (p, name) == 0)
		return (i + 1);
	    p += strlen (p) + 1;
	}
	return (-1);
    }
    else if (func == RMT_LHI_I2IX) {  		/* ip to index */
	unsigned int ip = *((unsigned int *)buf);
	ip = ntohl (ip);
	for (i = 0; i < n_hosts; i++) {
	    if (addrs[i] == ip)
		return (i + 1);
	}
	return (-1);
    }
    else if (func == RMT_LHI_IX2I) {		/* index to ip */
	unsigned int *ip = (unsigned int *)buf;
	if (index == 0)
	    index = *lix;
	if (index <= 0 || index > n_hosts)
	    return (-1);
	*ip = htonl (addrs[index - 1]);
	return (index);
    }
    else if (func == RMT_LHI_IX2H) {		/* index to host */
	char *p, **name = (char **)buf;
	p = names;
	if (index == 0)
	    index = *lix;
	if (index <= 0 || index > n_hosts)
	    return (-1);
	for (i = 1; i < index; i++)
	    p += strlen (p) + 1;
	*name = p;
	return (index);
    }
    else if (func == RMT_LHI_LIX) {		/* return local host index */
	return (*lix);
    }
    return (0);
}

/****************************************************************
			
	This function checks if the host "host_id" is in the client
	host list given in the RMT configuration file.

	It returns SUCCESS if the host name is in the list. If the host_id
	is an invalid host ID or the host name is not in the host
	list, the function returns FAILURE. Log messages are written to
	to the RMT log file on error conditions.
*/

static int
  Check_host_permission
  (
      unsigned int host_id	/* host ID to be checked */
) {
    int i;
    char buf[128];

    /* compare */
    for (i = 0; i < Client_cnt; i++) {
	if (Client_list[i].addr != WILD_ADDR) {
	    if (Client_list[i].addr == host_id)
		return (SUCCESS);
	}
	else {			/* compare to the wild card host entry */
	    int shift;
	    char tmp[64], *cpt;

	    strncpy (buf, Client_list[i].name, 127);
	    buf[127] = '\0';
	    cpt = strtok (buf, ". ");
	    shift = 24;
	    tmp[0] = '\0';
	    while (cpt != NULL && shift >= 0) {
		if (strcmp (cpt, "*") == 0)
		    strcat (tmp, "*");
		else
		    sprintf (tmp + strlen (tmp), "%d", 
					0xff & (host_id >> shift));
		if (shift > 0)
		    strcat (tmp, ".");
		shift -= 8;
		cpt = strtok (NULL, ". ");
	    }
	    if (strcmp (tmp, Client_list[i].name) == 0)
		return (SUCCESS);
	}
    }
    MISC_log ("Client %s not configured", NET_string_IP (host_id, 0, buf));  

    return (FAILURE);
}

/****************************************************************
			
	This function sends a handshaking message to the client.
	The message to be sent is of length "len" and located in 
	buffer "message". If the message can not be sent with 10 
	trials (about .2 seconds in total), the function will 
	give up and return. This is not considered a blocking
	function because, in normal cases, the write should 
	always finish with first write call given such a small
	message and an empty socket.

    Return:	SUCCESS or FAILURE.

*****************************************************************/

static int
  Send_auth_msg
  (
      int fd,			/* socket fd */
      int len,			/* message length */
      char *message		/* the message to be sent */
) {
    int n_sent;			/* number of bytes sent */
    int i;

    n_sent = 0;
    for (i = 0; i < 10; i++) {
	int k;			/* write return value */

	k = write (fd, &message[n_sent], len - n_sent);

	if (k < 0 && errno != EWOULDBLOCK)
	    return (FAILURE);

	if (k > 0)
	    n_sent += k;
	if (n_sent >= len)
	    return (SUCCESS);

	msleep (20);
    }

    /* failed in specified time period */
    return (FAILURE);
}

