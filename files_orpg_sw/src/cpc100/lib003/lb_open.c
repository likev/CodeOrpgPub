/****************************************************************
		
    Module: LB_open.c	
		
    Description: This module contains the LB_open, LB_close and
	LB_remove functions.

*****************************************************************/

/*
 * RCS info 
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/07/27 19:33:32 $
 * $Id: lb_open.c,v 1.101 2012/07/27 19:33:32 jing Exp $
 * $Revision: 1.101 $
 * $State: Exp $
 * $Log: lb_open.c,v $
 * Revision 1.101  2012/07/27 19:33:32  jing
 * Update
 *
 * Revision 1.96  2009/06/26 20:54:20  jing
 * Update
 *
 * Revision 1.75  2002/05/20 20:23:27  jing
 */


/* System include files */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

/* Local include files */

#include <lb.h>
#include <en.h>
#include <misc.h>
#include "lb_def.h"


/* array of lb structure pointers for open LBs */
static int Lb_array_size = 0;
static LB_struct **Lb_handle;
int LB_check_and_write = 0;

#ifdef LB_THREADED

#include <pthread.h>
static pthread_mutex_t Lb_open_mutex = PTHREAD_MUTEX_INITIALIZER;
			/*  mutex for LB_open/LB_close */
/* per thread data; Empty for the moment. We use this for thread termination 
   cleanup */
static pthread_key_t Ptd_key;
static pthread_once_t Key_init_once = {PTHREAD_ONCE_INIT};
static int Key_create_failed;
static void Key_init_func ();
static void Thread_cleanup (void *arg);
#endif

/* static functions */
static unsigned int 
    Get_sizes (int flags, int msg_size, int maxn_msgs, int tag_size,
	int n_nrs,
	int *n_slots, int *taga_size, int *cntr_size, int *ma_size, 
	int *msginfo_size, int *nra_size);
static int Initialize_lb_struct (LB_struct *lb);
static int Create_lb (const char *lb_name, LB_struct *lb, LB_attr *attr);
static int Open_lb (const char *lb_name, LB_struct *lb, LB_attr *attr);
static int Varify_header (char *lb_pt, int msg_size, int maxn_msgs, 
	int mode, int types, int attr_sizes);
static int Fill_file (int fd, int size);
static int Unlink_return (const char *name, int ret);
static int Open_lock_file (LB_struct *lb, const char *lb_name, 
					int open_flags, int open_mode);
static char *Lock_file_name (const char *lb_name);
static int Get_version_number (lb_t lbid);
static int Close_lb_return (LB_struct *lb, int ret);
static int Get_nra_size (int attr_tag_size);
static void Byte_swap_header (char *buf);
static void Byte_swap_control_area (char *buf, int n_slots, int taga_size);
static void Free_lb_resources (LB_struct *lb);
static int Allocate_lb_struct (const char *lb_name, LB_struct **lb_p);
static void Get_default_nra_size (LB_attr *attr);
static int Get_mmap_len (int size, int *p_size);
static int Unlock_close_lb_return (LB_struct *lb, int ret);
static key_t my_ftok (const char *pathname);
static int Get_lb_size (LB_struct *lb);


/********************************************************************
			
    Description: This function opens (or creates) an LB and get 
		an LB descriptor for later access. 

    Input:	lb_name - the LB name;
		flags - the LB open flags;
		attr - LB attributes;

    Returns:	This function returns an LB descriptor on success 
		or a negative number for indicating an error 
		condition. 

    Notes:	The function opens, or creates if needed, the medium. 
		It then allocates an LB structure and initializes it. 
		Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int 
LB_open (
    const char *lb_name,/* the LB name */
    int flags, 		/* the LB open flags */
    LB_attr *attr	/* LB attributes */
)
{
    LB_struct *lb;	/* LB structure */
    Lb_header_t *hd;
    int ret, index;
    unsigned int lbsize;
    char name_buf[256];

    if (lb_name == NULL || lb_name [0] == '\0')
	    return (LB_BAD_ARGUMENT);
    lb_name = MISC_expand_env (lb_name, name_buf, 256);

    if (attr != NULL) {
	if ((attr->types & (~ALL_TYPE_FLAGS)) != 0)
	    return (LB_BAD_ARGUMENT);
	if (attr->types & (LB_REPLACE | LB_MSG_POOL))
	    attr->types |= LB_DB;
	if ((attr->tag_size >> NRA_SIZE_SHIFT) == LB_DEFAULT_NRS)
	    Get_default_nra_size (attr);
    }

    if (flags & LB_CREATE) 
	flags |= LB_WRITE;	/* switch on the LB_WRITE flag */

#ifdef LB_THREADED
    pthread_once (&Key_init_once, Key_init_func);
    if (Key_create_failed)
	return (LB_PTHREAD_KEY_CREATE_FAILED);
    pthread_setspecific (Ptd_key, (void *)&ret);
				/* set to any non NULL value */
    pthread_mutex_lock (&Lb_open_mutex);
#endif

    lb = NULL;
    index = Allocate_lb_struct (lb_name, &lb);
    if (index < 0) {
#ifdef LB_THREADED
	pthread_mutex_unlock (&Lb_open_mutex);
#endif
	return (index);
    }

    lb->flags = flags;
    if ((flags & LB_CREATE) && lb->pld->use_cnt == 0)
	ret = Create_lb (lb_name, lb, attr);
    else
	ret = Open_lb (lb_name, lb, attr);

    /* initialize the LB structure */
    if (ret == LB_SUCCESS) 
	ret = Initialize_lb_struct (lb);

    /* free the tmp buffer allocated in Create_lb or Open_lb */
    if (lb->pld->use_cnt == 0 && lb->pld->lb_pt != NULL) {
	if (lb->flags & LB_MEMORY)
	    shmdt ((char *)lb->pld->lb_pt);
	else
	    free ((char *)lb->pld->lb_pt);
	lb->pld->lb_pt = NULL;
    }

    if (ret < 0)
	return (Close_lb_return (lb, ret));

    /* initialize the read pointer; we have to read hd from the LB; */
    if ((ret = LB_lock_mmap (lb, READ_PERM, NO_LOCK)) < 0)
	return (Unlock_close_lb_return (lb, ret));

    hd = lb->hd;
    if ((lb->flags & LB_MUST_READ) && lb->hd->non_dec_id <= 1 && 
					!(lb->flags & LB_SHARE_STREAM))
	lb->rpt = hd->ptr_read;
    else
	lb->rpt = GET_POINTER (hd->num_pt);
    lb->rpage = LB_Get_corrected_page_number (lb, (int)lb->rpt, 
				(int)(hd->ptr_page));

    /* initialize lb update status info */
    ret = LB_stat_init (lb);
    if (ret < 0)
	return (Unlock_close_lb_return (lb, ret));

    /* verify LB size; This is a check for flags, msg_size and maxn_msgs;
       This is necessary to protect from later out-of-boundary crash. */
    if (lb->flags & LB_MEMORY) {
	struct shmid_ds buf;
	shmctl(lb->pld->shmid, IPC_STAT, &buf);
	lbsize = buf.shm_segsz;
#ifdef TEMP_LINUX_BUG_FIX
	lbsize = *((int *)((char *)&buf + 16));
	/* for some LINUX installation, struct shmid_ds is not correct */
#endif
    }
    else {
	lbsize = lseek (lb->pld->fd, 0, SEEK_END);
    }
    ret = Get_sizes (lb->flags, hd->msg_size, 
		lb->hd->n_msgs, hd->tag_size, hd->nra_size,
		NULL, NULL, NULL, NULL, NULL, NULL);
    if (lbsize < (unsigned int)ret)
	return (Unlock_close_lb_return (lb, LB_LB_ERROR));

    /* lock the file for LB_SINGLE_WRITER type */
    if ((lb->flags & LB_SINGLE_WRITER) && (lb->flags & LB_WRITE)) {

	ret = LB_process_lock (SET_LOCK, lb, EXC_LOCK, 
					LB_SINGLE_WRITE_LOCK_OFF);
	if (ret == LB_LOCKED || ret == LB_SELF_LOCKED)
	    return (Unlock_close_lb_return (lb, LB_TOO_MANY_WRITERS));
	else if (ret < 0)
	    return (Unlock_close_lb_return (lb, ret));
    }

    lb->pld->use_cnt++;

#ifdef LB_THREADED
    lb->thread_id = pthread_self ();
    pthread_mutex_unlock (&Lb_open_mutex);
#else
    lb->thread_id = 0;
#endif

    /* return the LB descriptor */
    EN_internal_block_NTF ();	/* to balance unblock in the next call */
    
    return (LB_Unlock_return (lb, index + MIN_INDEX));
}
	
/********************************************************************
			
    Description: Allocates a new LB struct "lb". 

    Input:	lb_name - name of the LB.

    Output:	lb_p - pointer to the new LB struct;

    Return:	The LB index on success or an LB error code.

********************************************************************/

static int Allocate_lb_struct (const char *lb_name, LB_struct **lb_p)
{
    int index, size;
    LB_struct *lb_to_share, *lb;
    char *cpt;

    /* get an available LB index */
    cpt = NULL;			/* not necesary - turn off gcc warning */
    while (1) {
	int i;
	index = -1;
	lb_to_share = NULL;
	for (i = 0; i < Lb_array_size; i++) {
	    if (Lb_handle[i] == NULL) {
		if (index < 0)
		    index = i;
	    }
	    else {
		LB_struct *lb;
		lb = Lb_handle[i];
		if (lb_to_share == NULL &&
		    strcmp (lb->pld->lb_name, lb_name) == 0)
		    lb_to_share = lb;
	    }
	}
	if (index < 0) {	/* realloc Lb_handle */
	    LB_struct **pt;
	    int i;

	    pt = (LB_struct **)malloc ((Lb_array_size + N_INCREMENT) *
							sizeof (LB_struct *));
	    if (pt == NULL)
		return (LB_MALLOC_FAILED);
	    if (Lb_handle != NULL) {
		memcpy ((char *)pt, (char *)Lb_handle, Lb_array_size * 
							sizeof (LB_struct *));
		free (Lb_handle);
	    }
	    Lb_handle = pt;
	    for (i = 0; i < N_INCREMENT; i++)
		Lb_handle[Lb_array_size + i] = NULL;
	    Lb_array_size += N_INCREMENT;
	}
	else
	    break;
    }

    /* allocate space for the handle */
    size = 0;
    if (lb_to_share == NULL)
	size += sizeof (Per_lb_data_t) + strlen (lb_name) + 1;
    if ((lb = (LB_struct *) malloc (sizeof (LB_struct))) == NULL ||
	(size > 0 && (cpt = (char *)malloc (size)) == NULL)) {
	Lb_handle[index] = NULL;		/* free the descriptor */
	if (lb != NULL)
	    free (lb);
	return (LB_MALLOC_FAILED);
    }

    Lb_handle[index] = lb;
    memset (lb, 0, sizeof (LB_struct));
    if (lb_to_share != NULL) {
	lb->pld = lb_to_share->pld;
    }
    else {
	Per_lb_data_t *pld;
	int ret;

	memset (cpt, 0, sizeof (Per_lb_data_t));
	lb->pld = (Per_lb_data_t *)cpt;
	cpt += sizeof (Per_lb_data_t);
	pld = lb->pld;
	pld->lb_name = cpt;
	strcpy (pld->lb_name, lb_name);
	pld->use_cnt = 0;
	pld->fd = pld->lockfd = -1;
	pld->shmid = -1;
	if ((ret = LB_init_lock_table (pld)) != 0)
	    return (ret);
	pld->rsid = NULL;
	pld->rsid_buf = (char *)malloc (RSIS_local_buf_size (2)); 
						/* two keys used */
	if (pld->rsid_buf == NULL)
	    return (LB_MALLOC_FAILED);
    }
    *lb_p = lb;

    return (index);
}

/********************************************************************
			
    Description: Frees the LB struct "lb" and removes the descriptor. 

    Input:	lb - the LB involved;

********************************************************************/

static void Free_lb_resources (LB_struct *lb)
{
    int index;

    LB_stat_free (lb);

    lb->pld->use_cnt--;

    if (lb != NULL && lb->pld->use_cnt <= 0) {

	LB_cleanup_lock_table (lb->pld);

	if (lb->flags & LB_MEMORY) {
	    if (lb->pld->lb_pt != NULL)
		shmdt ((char *)lb->pld->lb_pt); 
	}
	else {
	    LB_unmap (lb);
	}

	if (lb->pld->fd >= 0) {
	    MISC_close (lb->pld->fd);
	    if (lb->pld->lockfd == lb->pld->fd)
		lb->pld->lockfd = -1;
	    lb->pld->fd = -1;
	}
	if (lb->pld->lockfd >= 0) {
	    MISC_close (lb->pld->lockfd);
	    lb->pld->lockfd = -1;
	}
	free (lb->pld->rsid_buf);
	free (lb->pld);
    }
    for (index = 0; index < Lb_array_size; index++) {
	if (Lb_handle[index] == lb)
	    Lb_handle[index] = NULL;		/* free the descriptor */
    }
    if (lb != NULL)
	free ((char *)lb);
}

/********************************************************************
			
    Unlocks the LB exclusive access lock, closes the LB and returns
    value "ret". 

    Input:	lb - The LB struct;
		ret - return value;

    Returns: 	argument "ret".

********************************************************************/

static int Unlock_close_lb_return (LB_struct *lb, int ret) {

    if (lb->locked == LB_LB_LOCKED) {
	LB_process_lock (UNSET_LOCK, lb, 0, LB_EXCLUSIVE_LOCK_OFF);
	lb->locked = LB_UNLOCKED;
    }
#ifdef LB_THREADED
    pthread_mutex_unlock (&lb->pld->access_mutex);
#endif
    return (Close_lb_return (lb, ret));
}

/********************************************************************
			
    Description: This function closes LB "lb" and returns value
		"ret". 

    Input:	lb - The LB struct;
		ret - return value;

    Returns: 	argument "ret".

********************************************************************/

static int Close_lb_return (LB_struct *lb, int ret)
{

    lb->pld->use_cnt++;
    Free_lb_resources (lb);
#ifdef LB_THREADED
    pthread_mutex_unlock (&Lb_open_mutex);
#endif
    return (ret);
}
		
/********************************************************************
			
    Description: This function creates a new LB and initializes it. 

    Input:	lb_name - LB name;
		lb - The LB structure;
		attr - LB attribute structure;

    Output:	lb - fields are updated in this structure;

    Returns: 	This function returns LB_SUCCESS on success
		or a negative LB_open return value on failure.

********************************************************************/

static int Create_lb (const char *lb_name, LB_struct *lb, LB_attr *attr)
{
    int ret;
    unsigned int size;
    int cntr_size, ma_size;
    int w_size, nra_s;
    char *cpt;
    Lb_header_t *hd;
    lb_t lbidnum;
    Per_lb_data_t *pld;

    /* clean up the current LB */
    LB_remove (lb_name);

    if (attr == NULL || attr->msg_size < 0 || 
	attr->maxn_msgs <= 0 || attr->maxn_msgs > MAXN_MSGS ||
	(attr->tag_size >> NRA_SIZE_SHIFT) > LB_DEFAULT_NRS || 
	(attr->tag_size & TAG_SIZE_MASK) > LB_MAXN_TAG_BITS)
	return (LB_BAD_ARGUMENT);

    if (attr->msg_size == 0) {
	if (attr->types & LB_MEMORY)
	    return (LB_BAD_ARGUMENT);
    }

    if ((attr->types & (LB_DIRECT | LB_DB | LB_REPLACE | LB_MSG_POOL)) &&
	(attr->types & (LB_MUST_READ | LB_SHARE_STREAM)))
	return (LB_BAD_ARGUMENT);

    /* create the LB file */
    pld = lb->pld;
    pld->fd = MISC_open (lb_name, O_CREAT | O_RDWR, attr->mode);
    if (pld->fd < 0)
	return (LB_OPEN_FAILED);
    if (fchmod (pld->fd, attr->mode) < 0)
	return (LB_CHMOD_FAILED);
    fcntl (pld->fd, F_SETFD, FD_CLOEXEC);

    lb->flags |= attr->types;
    if ((ret = Open_lock_file (lb, lb_name, O_CREAT | O_RDWR, attr->mode))
					!= LB_SUCCESS)
	return (Unlink_return (lb_name, ret));

    lb->flags = SET_VERSION (lb->flags, Get_version_number (LB_IDNUMBER));
    nra_s = Get_nra_size (attr->tag_size);
    size = Get_sizes ((int)lb->flags, attr->msg_size, 
			attr->maxn_msgs, attr->tag_size & TAG_SIZE_MASK, 
			nra_s, NULL, NULL, &cntr_size, &ma_size, NULL, NULL);
    if (size > MAX_LB_SIZE)
	return (Unlink_return (lb_name, LB_MSG_TOO_LARGE));
    if (lb->flags & LB_MEMORY) {	/* create the shm */
	char buf[256];

	if (lb_name[0] != '/') {
	    if (getcwd (buf, 256) == NULL ||
		strlen (buf) + strlen (lb_name) + 2 > 256)
		return (LB_FTOK_FAILED);
	    strcat (buf, "/");
	    strcat (buf, lb_name);
	    lb->key = my_ftok (buf);
	}
	else
	    lb->key = my_ftok (lb_name);
	if ((int)(lb->key) == -1)	/* get a key */
	    return (Unlink_return (lb_name, LB_FTOK_FAILED));

	/* open shm */
	pld->shmid = shmget ((key_t)lb->key, size, attr->mode | IPC_CREAT);	
	if (pld->shmid < 0) 
	    return (Unlink_return (lb_name, LB_SHMGET_FAILED));

	pld->lb_pt = (char *)shmat (pld->shmid, (char *)0, 0); /* map to memory */
	if ((int) pld->lb_pt == -1) {
	    pld->lb_pt = NULL;
	    return (Unlink_return (lb_name, LB_MMAP_FAILED));
	}
    }
    else {				/* allocate temp buffer */

	pld->lb_pt = (char *)malloc (cntr_size);
	if (pld->lb_pt == NULL)
	    return (Unlink_return (lb_name, LB_MALLOC_FAILED));
    }

    /* initialize the LB */
    cpt = pld->lb_pt;
    hd = (Lb_header_t *)cpt; 
    memset (cpt, '\0', cntr_size);
    if (attr->remark != NULL)
	memcpy (hd->remark, attr->remark, LB_REMARK_LENGTH);
    hd->msg_size = attr->msg_size;
    hd->n_msgs = attr->maxn_msgs;
    hd->acc_mode = attr->mode;
    hd->lb_types = attr->types;
    hd->shm_key = lb->key;
    hd->ptr_page = 0;
    hd->num_pt = 0;
    hd->lb_time = time (NULL);
    hd->ptr_read = 0;
    hd->unused_bytes = 0;

    hd->tag_size = attr->tag_size & TAG_SIZE_MASK;
    hd->nra_size = Get_nra_size (attr->tag_size);
    hd->upd_flag = 0;
    hd->non_dec_id = 1;
    hd->sms_ok = hd->miscflags = 0;
    hd->unique_msgid = 0;
    hd->is_bigendian = MISC_i_am_bigendian ();
    hd->read_cnt = 0;

    hd->lb_id = 0;

    /* write to the file */
    lbidnum = LB_IDNUMBER;
    if (lb->flags & LB_MEMORY)
	w_size = sizeof (Lb_header_t);
    else {
	int ret;

	if ((ret = Fill_file (pld->fd, size)) != LB_SUCCESS) 
	    return (Unlink_return (lb_name, ret));
	w_size = cntr_size;
    }
    /* we update lb_id field later to protect the LB integrity */
    if (lseek (pld->fd, 0, SEEK_SET) < 0 ||
	MISC_write (pld->fd, cpt, w_size) != w_size ||
	lseek (pld->fd, (off_t)LB_REMARK_LENGTH, SEEK_SET) < 0 ||
	MISC_write (pld->fd, (char *)&lbidnum, 
				sizeof (lb_t)) != sizeof (lb_t))
	return (Unlink_return (lb_name, LB_UPDATE_FAILED));

    hd->lb_id = LB_IDNUMBER;
#ifdef LINUX
    fdatasync (pld->fd);
#endif

    return (LB_SUCCESS);
}
		
/********************************************************************
			
    Description: This function returns the nra_size value of LB header. 

    Input:	attr_tag_size - the tag_size field in LB_attr.

    Returns: 	the nra_size value of LB header.

********************************************************************/

static int Get_nra_size (int attr_tag_size)
{

    return (((attr_tag_size >> NRA_SIZE_SHIFT) + LB_NR_FACTOR - 1) / 
							LB_NR_FACTOR);
}

/***********************************************************************

    Converts LB_DEFAULT_NRS to the default nra size value.

***********************************************************************/

static void Get_default_nra_size (LB_attr *attr) {

    int nra_size = attr->maxn_msgs * 4;
    if (attr->types & LB_DB) {
	if (nra_size > 100)
	    nra_size = 100;
    }
    else {
	if (nra_size > 32)
	    nra_size = 32;
    }
    attr->tag_size = (attr->tag_size & TAG_SIZE_MASK) | 
					(nra_size << NRA_SIZE_SHIFT);
}

/********************************************************************
			
    Description: This function removes file "name" and returns the
		argument "ret". 

    Input:	name - name of file to be removed
		ret - value returned by this function

    Returns: 	This function returns the argument "ret".

********************************************************************/

static int Unlink_return (const char *name, int ret)
{

    MISC_unlink (name);
    return (ret);
}
 	
/********************************************************************
			
    Description: This function writes "size" bytes or 0 to file fd. 

    Input:	fd - the file descriptor
		size - size of data to fill in.

    Returns: 	This function returns LB_SUCCESS on success
		or a negative error number on failure.

********************************************************************/

#define TMP_BUF_SIZE 8192

static int Fill_file (int fd, int size)
{
    char *buf;
    int *ipt;

    if ((buf = (char *)malloc (TMP_BUF_SIZE)) == NULL)
	return (LB_MALLOC_FAILED);

    ipt = (int *)buf;
    while (ipt < (int *)(buf + TMP_BUF_SIZE))
	*ipt++ = 0;

    while (1) {
	int n, ret;

	if (size <= 0)
	    break;
	n = size;
	if (n > TMP_BUF_SIZE)
	    n = TMP_BUF_SIZE;
	if ((ret = MISC_write (fd, buf, n)) != n) {
	    free (buf);
	    return (ret);
	}
	size -= n;
    }
    free (buf);
    return (LB_SUCCESS);
}
	
/********************************************************************
			
    Description: This function opens an existing LB and verifies it. 

    Input:	lb_name - LB name;
		lb - The LB structure;
		attr - LB attribute structure;

    Output:	t_buf - pointer to a temporary buffer;
		lb - fields are updated in theis structure;

    Returns: 	This function returns LB_SUCCESS on success
		or a negative LB_open return value on failure.

********************************************************************/

static int Open_lb (const char *lb_name, LB_struct *lb, LB_attr *attr)
{
    int msg_size, maxn_msgs, mode, types, attr_sizes;
    char *t_buf;
    int ret;
    Lb_header_t *hd;
    Per_lb_data_t *pld;

    if (attr != NULL) {
	msg_size = attr->msg_size;
	maxn_msgs = attr->maxn_msgs;
	mode = attr->mode;
	types = attr->types;
	attr_sizes = attr->tag_size;
    }
    else 
	msg_size = maxn_msgs = mode = types = attr_sizes = 0;

    /* open the file */
    pld = lb->pld;
    if (pld->use_cnt == 0 &&
	(pld->fd = MISC_open (lb_name, O_RDWR, 0)) < 0)
	return (LB_OPEN_FAILED);
    fcntl (pld->fd, F_SETFD, FD_CLOEXEC);

    /* read in the header */
    if (pld->use_cnt == 0) {
	t_buf = (char *)malloc (sizeof (Lb_header_t));
	if (t_buf == NULL)
	    return (LB_MALLOC_FAILED);
	if (MISC_read (pld->fd, t_buf, sizeof (Lb_header_t)) 
					!= sizeof (Lb_header_t)) {
	    free (t_buf);
	    return (LB_LB_ERROR);
	}
	ret = 0;
	if (Get_version_number (((Lb_header_t *)t_buf)->lb_id) < 0)
	    ret = LB_NON_LB_FILE;
	if (ret >= 0 &&
		((Lb_header_t *)t_buf)->is_bigendian != MISC_i_am_bigendian ())
	    ret = LB_BAD_BYTE_ORDER;
	if (ret >= 0)
	    ret = Varify_header (t_buf, msg_size, 
				maxn_msgs, mode, types, attr_sizes);
	if (ret < 0) {
	    free (t_buf);
	    return (ret);
	}
	hd = (Lb_header_t *)t_buf;
    }
    else {
	hd = (Lb_header_t *)pld->lb_pt;
	t_buf = NULL;
    }
	
    msg_size = hd->msg_size;
    maxn_msgs = hd->n_msgs;
    mode = hd->acc_mode;
    types = hd->lb_types;
    lb->flags |= types;

    if (pld->use_cnt > 0)
	return (LB_SUCCESS);

    if ((ret = Open_lock_file (lb, lb_name, O_RDWR, 0)) != LB_SUCCESS) {
	if (t_buf != NULL)
	    free (t_buf);
	return (ret);
    }

    if (lb->flags & LB_MEMORY) {	/* open the shm */

	pld->shmid = shmget ((key_t)hd->shm_key, 0, 0);
	if (t_buf != NULL)
	    free (t_buf);
	if (pld->shmid < 0)
	    return (LB_SHMGET_FAILED);
	pld->lb_pt = (char *)shmat (pld->shmid, (char *)0, 0);
	if ((int)pld->lb_pt == -1) {
	    pld->lb_pt = NULL;
	    return (LB_MMAP_FAILED);
	}

	/* verify the header area */
	ret = Varify_header (pld->lb_pt, msg_size, maxn_msgs, 
						mode, types, attr_sizes);
	if (ret < 0)
	    return (ret);
    }
    else
	pld->lb_pt = t_buf;

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function verifies an LB header.

    Input:	lb_pt - point to the LB header area.
		msg_size - message size
		maxn_msgs - maximum number of messages
		mode - access permission
		types - LB type flags
		attr_sizes - tag size and nra size as in attr->tag_size.

    Return:	It returns LB_SUCCESS on success or LB_DIFFER
		if the LB header is found to be non-LB file or
		the attributes are different.

********************************************************************/

static int Varify_header (char *lb_pt, int msg_size, int maxn_msgs, 
	int mode, int types, int attr_sizes)
{
    Lb_header_t *hd;
    int ver;
    int t_s, w_s;

    hd = (Lb_header_t *)lb_pt;
    t_s = attr_sizes & TAG_SIZE_MASK;
    w_s = Get_nra_size (attr_sizes);
    if (hd->lb_types & (LB_REPLACE | LB_MSG_POOL))
	hd->lb_types |= LB_DB;
    if (types & (LB_REPLACE | LB_MSG_POOL))
	types |= LB_DB;

    if ((ver = Get_version_number (hd->lb_id)) < 5 || ver > 6)
	return (LB_VERSION_NOT_SUPPORTED);

    if ((msg_size != 0 && msg_size != hd->msg_size) ||
	(maxn_msgs != 0 && maxn_msgs != hd->n_msgs) ||
	(types != 0 && types != hd->lb_types) ||
	(mode != 0 && mode != hd->acc_mode) ||
	(ver > 2 && 
		((t_s != 0 && t_s != hd->tag_size) ||
		 (w_s != 0 && w_s != hd->nra_size))   ))
	return (LB_DIFFER);
    else
	return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function removes LB "lb_name". It first
		removes the shared memory segment if the LB is a 
		shared memory LB. Then it removes the LB files. If
		the file is not an LB file or a corrupted LB file,
		it is removed.

    Input:	lb_name - name of the LB

    Return:	It returns LB_SUCCESS on success or negative LB error
		number. Note that, if file "lb_name" does not exist, 
		it returns an error code.

********************************************************************/

int LB_remove (const char *lb_name)
{
    Lb_header_t hd;
    int fd;
    char *name, name_buf[256];

    if (lb_name == NULL || strlen (lb_name) == 0)
	return (LB_BAD_ARGUMENT);
    lb_name = MISC_expand_env (lb_name, name_buf, 256);

    fd = MISC_open (lb_name, O_RDWR, 0);/* open the file to find the key */
    if (fd < 0) {
	if (errno == ENOENT)
	    return (LB_NOT_EXIST);
    }
    else {
	MISC_log_disable (1);
	if (MISC_read (fd, (char *)&hd, sizeof (Lb_header_t)) == 
						sizeof (Lb_header_t) &&
	    Get_version_number (hd.lb_id) >= 0 &&
	    (hd.lb_types & LB_MEMORY)) {		/* remove the shm */
	    struct shmid_ds tbuf;
	    int id;
    
	    id = shmget ((key_t)hd.shm_key, 0, 0);
	    if (id >= 0)
		shmctl (id, IPC_RMID, &tbuf);
	}
	MISC_close (fd);
	MISC_log_disable (0);
    }

    if ((name = Lock_file_name (lb_name)) != NULL) {
	MISC_unlink (name);
	free (name);
    }
    if (MISC_unlink (lb_name) < 0) {
	if (errno == ENOENT)
	    return (LB_SUCCESS);
	else
	    return (LB_REMOVE_FAILED);
    }

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function registers a user address in LB "lbd"
		for passing certain variables.

    Input:	lbd - LB descriptor;
		type - the type of address to register;
		address - the address to register;

    Returns:	returns LB_SUCCESS on success or a negative 
		number to indicate an error condition.

    Notes:	Refer to lb.doc for a detailed description of this 
		function.

********************************************************************/

int LB_register (int lbd, int type, void *address)
{
    LB_struct *lb;
    int ret;

    /* get the LB structure */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);

    switch (type) {
	case LB_ID_ADDRESS: 
	    lb->umid = (LB_id_t *)address;
	    break;

	case LB_TAG_ADDRESS:
	    lb->utag = (int *)address;
	    break;

	default:
	    break;
    }

    LB_reset_inuse (lb);
    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This implements various miscellaneous LB functions. 

    Input:	lbd - LB descriptor
		cmd - command

    Returns:	This function returns LB_SUCCESS on success, or a 
		negative number to indicate an error condition.

********************************************************************/

int LB_misc (int lbd, int cmd)
{
    LB_struct *lb;
    int ret, parm;

    /* get the LB structure, lock and mmap the file */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);

    parm = 0;
    if (cmd >> 16) {
	parm = cmd & 0xffff;
	cmd = cmd >> 16;
    }
    switch (cmd) {
	case LB_GAIN_EXCLUSIVE_LOCK:
	    if (!lb->lb_misc_locked) {
		if ((ret = LB_process_lock (SET_LOCK_WAIT, lb, EXC_LOCK, 
				    LB_EXCLUSIVE_LOCK_OFF)) < 0)
		    return (ret);
		lb->locked = LB_LB_LOCKED;
		lb->lb_misc_locked = 1;
	    }
	    lb->inuse = 0;
	    EN_internal_unblock_NTF ();
	    return (LB_SUCCESS);
	case LB_RELEASE_EXCLUSIVE_LOCK:
	    if (lb->lb_misc_locked) {
		LB_process_lock (UNSET_LOCK, lb, 0, LB_EXCLUSIVE_LOCK_OFF);
		lb->locked = LB_UNLOCKED;
		lb->lb_misc_locked = 0;
	    }
	    lb->inuse = 0;
	    EN_internal_unblock_NTF ();
	    return (LB_SUCCESS);
	case LB_IS_BIGENDIAN:
	    lb->inuse = 0;
	    EN_internal_unblock_NTF ();
	    return (lb->hd->is_bigendian);
	case LB_CHECK_AND_WRITE:
	    LB_check_and_write = 1;
	    lb->inuse = 0;
	    EN_internal_unblock_NTF ();
	    return (0);
	default:
	    break;
    }

    if ((ret = LB_lock_mmap (lb, WRITE_PERM, EXC_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));

    switch (cmd) {

	case LB_SET_ACTIVE_TEST:
	    lb->active_test = 1;
	    break;
	case LB_UNSET_ACTIVE_TEST:
	    lb->active_test = 0;
	    break;
	case LB_SET_ACTIVE_LOCK:
	    ret = LB_process_lock (SET_LOCK, lb, 
				EXC_LOCK, LB_ACTIVE_SV_LOCK_OFF);
	    if (ret == LB_LOCKED || ret == LB_SELF_LOCKED)
		return (LB_Unlock_return (lb, LB_HAS_BEEN_LOCKED));
	    if (ret < 0)
		return (LB_Unlock_return (lb, ret));
	    break;
	case LB_UNSET_ACTIVE_LOCK:
	    LB_process_lock (UNSET_LOCK, lb, 
				EXC_LOCK, LB_ACTIVE_SV_LOCK_OFF);
	    break;

	case LB_N_MUST_READERS:
	    if (!(lb->flags & LB_MUST_READ) || (lb->flags & LB_SHARE_STREAM) ||
				    parm > 127 || parm < 0)
		return (LB_Unlock_return (lb, LB_BAD_ARGUMENT));
	    lb->hd->non_dec_id = (parm << 1) | (lb->hd->non_dec_id & 1);
	    lb->hd->read_cnt = 0;    
	    lb->hd->ptr_read = GET_POINTER (lb->hd->num_pt);
	    break;

	case LB_DISCARD_UNREAD_IN_SHARED_STREAM:
	    if (lb->flags & LB_SHARE_STREAM)
		lb->hd->ptr_read = GET_POINTER (lb->hd->num_pt);
	    break;
	case LB_UNSET_DERECT_ACCESS:
	    return (LB_Unlock_return (lb, 
			LB_direct_access_lock (lb, UNSET_LOCK, 0)));
	case LB_GET_UNIQUE_MSGID:
	    ret = lb->hd->unique_msgid;
	    lb->hd->unique_msgid = ret + 1;
	    return (LB_Unlock_return (lb, ret));
	case LB_GET_LB_SIZE:
	    return (LB_Unlock_return (lb, Get_lb_size (lb)));
	case LB_VALIDATE_HEADERS:
	    return (LB_Unlock_return (lb, LB_sms_validate_header (lb)));
	default:
	    return (LB_Unlock_return (lb, LB_MISC_BAD_CMD));
    }

    return (LB_Unlock_return (lb, LB_SUCCESS));
}

/********************************************************************
			
    Reads(func = 0)/writes(func != 0) the sdqs port number and IP 
    address from/to the LB "lbd". Returns LB_SUCCESS on success, or a 
    negative error number.

********************************************************************/

int LB_sdqs_address (int lbd, int func, int *port, unsigned int *ip) {
    LB_struct *lb;
    int ret;

    /* get the LB structure, lock and mmap the file */
    lb = LB_Get_lb_structure (lbd, &ret);
    if (lb == NULL) 
	return (ret);
    if ((ret = LB_lock_mmap (lb, WRITE_PERM, EXC_LOCK)) < 0)
	return (LB_Unlock_return (lb, ret));
    if (func == 0) {		/* read port and IP */
	*port = lb->hd->sdqs_port;
	*ip = lb->hd->sdqs_ip;
    }
    else {			/* update port and IP */
	lb->hd->sdqs_port = *port;
	lb->hd->sdqs_ip = *ip;
    }
    return (LB_Unlock_return (lb, LB_SUCCESS));
}

/********************************************************************
			
    Description: This function initializes a new LB structure. 

    Input:	fd - File fd or the shm id;
		hd - Pointer to the LB header area;
		flags - The LB flags;

    Output:	lb - The LB structure;

    Notes:	A msg pointer (read or write) is in the range of 0 through 
		ptr_range - 1. ptr_range must be an integer multiple of n_msgs 
		and at least 4 times of n_msgs. A page number can be positive 
		or negative. Every time a pointer moves across 
		(ptr % page_size) = page_size - 1 and 0, the corresponding page 			number is changed by 1 or -1. We use N_PAGES = 4 and 

	page_size = maxn_msgs * (POINTER_RANGE / (N_PAGES * maxn_msgs)) and
	ptr_range = N_PAGES * page_size

		This will satisfy the page size requirements discussed earlier 
		and be as large as possible. The write page number is always 
		increasing after the LB is created. It takes virtually 
		infinitely long time for a page number to reach the maximum 
		value. We do not need to process the page overflow.

    Return:	This function returns LB_SUCCESS or an LB error number.

********************************************************************/

static int Initialize_lb_struct (LB_struct *lb)
{
    Lb_header_t *hd;
    int n_nrs, mlen;
    int n_slots, taga_size, control_size, ma_size, msginfo_size, nra_size;
    Per_lb_data_t *pld;

    pld = lb->pld;
    hd = (Lb_header_t *)pld->lb_pt;
    if (pld->use_cnt == 0) {
	lb->hd = hd;
	lb->dir = NULL;
	lb->msginfo = NULL;
	lb->tag = NULL;
	pld->map_off = pld->map_len = pld->cw_perm = pld->dw_perm = 0;
    }
    lb->unused[0] = lb->unused[1] = lb->unused[2] = 0;

    lb->locked = LB_UNLOCKED;

    lb->flags = 
	SET_VERSION (lb->flags, Get_version_number (hd->lb_id));
    n_nrs = hd->nra_size;
    Get_sizes (lb->flags, hd->msg_size, hd->n_msgs, hd->tag_size,
			n_nrs, &n_slots, &taga_size, &control_size, &ma_size, 
			&msginfo_size, &nra_size);

    lb->n_slots = n_slots;
    lb->ma_size = ma_size;
    lb->maxn_msgs = hd->n_msgs;
    lb->off_a = control_size;
    lb->off_tag = sizeof (Lb_header_t) + n_slots * sizeof (LB_dir);
    lb->off_msginfo = lb->off_tag + taga_size;
    lb->off_nra = lb->off_msginfo + msginfo_size;
    lb->off_sms = lb->off_nra + nra_size;
    if (n_nrs == 0)
	lb->off_nra = 0;
    mlen = Get_mmap_len (lb->off_a, &(lb->mpg_size));

    lb->page_size = lb->n_slots * (POINTER_RANGE / (N_PAGES * lb->n_slots));
    lb->ptr_range = lb->page_size * N_PAGES;

    if (pld->use_cnt == 0) {
	pld->map_pt = NULL;
	pld->cntr_mlen = mlen;
    }

    lb->utag = NULL;
    lb->umid = NULL;
    lb->partial = 0;
    LB_Set_parity (lb);

    lb->prev_id = LB_PREV_MSGID_FAILED;
    lb->urhost = 0;
    lb->nr_lock_check_time = 0;
    lb->max_poll = 0;
    lb->read_offset = 0;
    lb->read_size = 0;
    lb->inuse = 0;
    lb->upd_flag_check = 0;
    lb->upd_flag_set = 0;
    lb->active_test = 0;
    lb->direct_access = 0;
    lb->maint_param = 0;
    lb->rpt_incd = -1;

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function closes an LB descriptor and frees all 
		associated resources. 

    Input:	lbd - the LB descriptor;

    Returns: 	This function returns LB_SUCCESS on success, or 
		LB_BAD_DESCRIPTOR if "lbd" is not a valid LB descriptor.

********************************************************************/

int 
LB_close (
    int lbd		/* lb descriptor */
)
{
    LB_struct *lb;
    int err;

#ifdef LB_THREADED
    pthread_mutex_unlock (&Lb_open_mutex);
#endif

    EN_close_notify (lbd);		/* rm all update notifications */

    err = 0;				/* this is needed since the next func
					   will set it only on failure */
    lb = LB_Get_lb_structure (lbd, &err); /* get lb structure */
    if (lb != NULL) {
	if (lb->flags & LB_DIRECT)
	    LB_direct_access_lock (lb, UNSET_LOCK, 0);
	LB_cleanup_locks (lb);
	Free_lb_resources (lb);
    }

#ifdef LB_THREADED
    pthread_mutex_unlock (&Lb_open_mutex);
#endif

    if (err < 0)
	return (err);

    EN_internal_unblock_NTF ();
    return (LB_SUCCESS);
}
	
/********************************************************************
			
    Description: This function computes the sizes of the parts in the
		LB control area. In the LB control area we have the
		following parts: LB header, message directory, tag
		area, msginfo area, notification request area (nra)
		and the free space control area.

    Input:	flags - the LB open flags;
		msg_size - average message size
		maxn_msgs - max number of messages
		tag_size - tag size
		n_nrs - notification request record number.

    output:	n_slots - number of dir slots
		taga_size - number of bytes for the tag area.
		cntr_size - control area size
		ma_size - message area size (single buffer)
		msginfo_size - msg info area size
		nra_size - notification request area size

    Returns:	This function returns the total size of the LB. 

    Notes:	Refer to design documents for the formulas used in 
		the function.

********************************************************************/

static unsigned int 
    Get_sizes (int flags, int msg_size, int maxn_msgs, int tag_size,
	int n_nrs,
	int *n_slots, int *taga_size, int *cntr_size, int *ma_size, 
	int *msginfo_size, int *nra_size)
{
    int nsl, tag_s, cntr, size, msginfo_s, nra_s;
    unsigned int total;

    /* number of slots */
    nsl = maxn_msgs + 4;
    if (n_slots != NULL)
	*n_slots = nsl;

    /* space for the tag area (in number of bytes) */
    {
	int word_s;

	word_s = sizeof (lb_t) * CHAR_SIZE;	/* lb_t word size */
	if (tag_size <= 0 || tag_size > word_s)
	    tag_s = 0;
	else {
	    int npw, nw;

	    npw = word_s / tag_size;		/* number of tags per word */
	    nw = (nsl + npw - 1) / npw;
	    tag_s = nw * sizeof (lb_t);
	}
	if (flags & LB_DB)
	    tag_s *= 2;				/* double buffered */
    }
    if (taga_size != NULL)
	*taga_size = tag_s;

    if (flags & LB_DB)
	msginfo_s = 2 * nsl * sizeof (LB_msg_info_t);/* size of msg info area, 
						   double buffered */
    else if (msg_size == 0)
	msginfo_s = nsl * sizeof (LB_msg_info_seq_t);/* size of msg info area, 
						   single buffered */
    else
	msginfo_s = 0;
    if (msginfo_size != NULL)
	*msginfo_size = msginfo_s;

    n_nrs *= LB_NR_FACTOR;
    nra_s = n_nrs * sizeof (LB_nr_t);
    if (nra_size != NULL)
	*nra_size = nra_s;

    /* control area size */
    cntr = sizeof (Lb_header_t) + nsl * sizeof (LB_dir) + 
					tag_s + msginfo_s + nra_s;
    if ((flags & LB_DB) || msg_size == 0)
	cntr += LB_sms_cntl_size (maxn_msgs, GET_VERSION (flags));
    if ((flags & LB_DIRECT) && !(flags & LB_MEMORY))
	cntr = Get_mmap_len (cntr, NULL);
    if (cntr_size != NULL)
	*cntr_size = cntr;

    /* message area size */
    size = msg_size * maxn_msgs;
    if (flags & LB_DB)
	size += msg_size;
    if (ma_size != NULL)
	*ma_size = size;

    total = cntr + size;
    return (total);
}
	
/********************************************************************
			
    Description: This function returns the LB structure of LB "lbd".

    Input:	lbd - the LB descriptor;

    output:	err - error number when failed.

    Returns:	This function returns the pointer to the structure on 
		success, or NULL on failure.

    Notes: 	This is a common function. We put it here because it 
		needs to access Lb_handle.

********************************************************************/

LB_struct *LB_Get_lb_structure (
    int lbd,		/* lb descriptor */
    int *err
)
{
    LB_struct *lb;

    if (lbd - MIN_INDEX < 0 || lbd - MIN_INDEX >= Lb_array_size || 
	Lb_handle[lbd - MIN_INDEX] == NULL) {
	*err = LB_BAD_DESCRIPTOR;
	return (NULL);
    }
    lb = Lb_handle[lbd - MIN_INDEX];

    EN_internal_block_NTF ();

    if (lb->inuse) {
	*err = LB_INUSE;
	EN_internal_unblock_NTF ();
	return (NULL);
    }
    else
	lb->inuse = 1;

    return (lb);
}
	
/********************************************************************
			
    Description: This function opens the external lock file if it 
		exists. Otherwise the LB file is used as the lock
		file. If the open_flags is O_CREAT, a lock test is
		performed to determine whether a lock file is needed.
		If it is needed, the function creates it. For opening
		an existing LB, we always first try to open the 
		external lock file.

    Input:	lb - the LB structure;
		lb_name - name of the LB.
		open_flags - flags for open.
		open_mode - permission mode for open.

    Return:	LB_SUCCESS on success or an negative error number on
		failure.

********************************************************************/

static int Open_lock_file (LB_struct *lb, const char *lb_name, 
					int open_flags, int open_mode)
{
    Per_lb_data_t *pld;

    pld = lb->pld;
    pld->lockfd = pld->fd;		/* by default */
    if (!(lb->flags & LB_MEMORY)) {
	int need_open;
	char *name;

	need_open = 0;
	if (open_flags & O_CREAT) {
	    if (LB_test_file_lock (pld->fd, lb->flags))
		need_open = 1;
	}
	else
	    need_open = 1;

	if (need_open) {
	    if ((name = Lock_file_name (lb_name)) == NULL) {
		return (LB_MALLOC_FAILED);
	    }
	    pld->lockfd = MISC_open (name, open_flags, open_mode);
	    free (name);
	    if (pld->lockfd < 0) {
		if (open_flags & O_CREAT)
		    return (LB_OPEN_LOCK_FILE_FAILED);
		else {
		    if (errno != ENOENT)	/* file exists */
			return (LB_OPEN_LOCK_FILE_FAILED);
		}
		pld->lockfd = pld->fd;
	    }
	    else {
		if ((open_flags & O_CREAT) && 
				fchmod (pld->lockfd, open_mode) < 0)
		    return (LB_CHMOD_FAILED);
		fcntl (pld->lockfd, F_SETFD, FD_CLOEXEC);
	    }
	}
    }

    return (LB_SUCCESS);
}
	
/********************************************************************
			
    Description: This function returns the external lock file name.
		The caller should free the memory allocated.

    Input:	lb_name - name of the LB.

    Return:	pointer to the lock file name on success or NULL on
		failure.

********************************************************************/

static char *Lock_file_name (const char *lb_name)
{
    char *name;

    name = (char *)malloc (strlen (lb_name) + 6);
    if (name == NULL)
	return (NULL);

    strcpy (name, lb_name);
    strcat (name, ".lock");

    return (name);
}

/********************************************************************
			
    Description: This function verifies the LB ID and returns the LB
		version number if "lbid" is valid.

    Input:	lbid - the LB ID.

    Return:	LB version number on success or -1 on failure.

********************************************************************/

static int Get_version_number (lb_t lbid)
{
    lb_t id, swapped_lbid;
    int i;

    id = LB_IDNUMBER;
    swapped_lbid = LB_byte_swap (lbid);
    for (i = 0; i < LB_N_VERSIONS; i++) {
	if (id == lbid || id == swapped_lbid)
	    return (9 - ((id / 1000) % 10));	/* see comments in lb_def.h */
	id += 1000;
    }
    return (-1);
}

/********************************************************************
			
    Description: This function computes and returns the actual space 
		used by the LB starting from the beginning of the 
		file (or shared memory)

    Input:	lb - the LB structure;

    Returns:	This function returns the actual space used by the LB 
		or a negative error number on failure. 

********************************************************************/

static int Get_lb_size (LB_struct *lb)
{
    unsigned int num_ptr;
    LB_dir *dir;
    int ptr, nmsgs, n_slots, pt;
    int free_o, msg_o;
    int size;

    if ((lb->flags & LB_DB) || lb->ma_size == 0)
	return (lb->off_a + LB_sms_space_used (lb));

    num_ptr = lb->hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    ptr = GET_POINTER(num_ptr);		/* pointer to the next new message */

    n_slots = lb->n_slots;
    dir = lb->dir;

    /* offset of the first message and the free space */
    ptr--;				/* pointer to the latest msg */
    if (ptr < 0)
	ptr = lb->ptr_range - 1;
    free_o = dir[ptr % n_slots].loc;
    pt = (ptr - nmsgs + lb->ptr_range) % lb->ptr_range;	/* the one before the 
							   first msg */
    msg_o = dir[pt % n_slots].loc;

    if (free_o > msg_o)		/* free space size */
	size = lb->off_a + free_o;
    else {
	if (free_o < msg_o)
	    size = lb->off_a + lb->ma_size;
	else {			/* free_o == msg_o */
	    if (nmsgs == 0)
		size = lb->off_a;
	    else
		size = lb->off_a + lb->ma_size;
	}
    }

    return (size);
}

/*********************************************************************

    Returns the mmap size of "size". The page size is returned with 
    "p_size" if it is not NULL, 

*********************************************************************/

static int Get_mmap_len (int size, int *p_size) {
    int pg_size;

#if (defined (SUNOS) || defined (LINUX) || defined(__WIN32__))
    pg_size = getpagesize ();
#else
    pg_size = sysconf (_SC_PAGE_SIZE);
#endif
    if (p_size != NULL)
	*p_size = pg_size;
    return (((size + pg_size - 1)/ pg_size) * pg_size);
}
		
/********************************************************************
			
    Description: This function reads the control area of an LB, 
		performs byte swaps and writes back to the LB. If
		this function is terminated before it completes 
		writing back, the LB is corrupted.

    Input:	lb_name - name of the LB.

    Return:	LB_SUCCESS or an LB error number.

********************************************************************/

int LB_fix_byte_order (char *lb_name)
{
    int fd, size, msginfo_size, taga_size, n_slots, swap_area_size;
    Lb_header_t hd, *hdp;
    int ret;
    char i_am_bigendian, *buf, bad_be, name_buf[256];

    /* open the file */
    lb_name = MISC_expand_env (lb_name, name_buf, 256);
    if ((fd = MISC_open (lb_name, O_RDWR, 0)) < 0)
	return (LB_OPEN_FAILED);

    /* read in the header */
    if (MISC_read (fd, (char *)&hd, sizeof (Lb_header_t)) != 
					sizeof (Lb_header_t))
	return (LB_LB_ERROR);

    if (hd.is_bigendian == MISC_i_am_bigendian ())
	return (LB_SUCCESS);
    if (hd.is_bigendian != 0 && hd.is_bigendian != 1)
	return (LB_LB_ERROR);

    Byte_swap_header ((char *)&hd);
    if (hd.lb_types & LB_MEMORY)
	return (LB_NOT_SUPPORTED);

    ret = Varify_header ((char *)&hd, hd.msg_size, hd.n_msgs, 
				hd.acc_mode, hd.lb_types, hd.tag_size);
    if (ret < 0)
	return (ret);

    size = Get_sizes (hd.lb_types, hd.msg_size, 
			hd.n_msgs, hd.tag_size, 
			hd.nra_size, &n_slots, &taga_size, NULL, 
			NULL, &msginfo_size, NULL);

    swap_area_size = sizeof (Lb_header_t) + 
		n_slots * sizeof (LB_dir) + taga_size + msginfo_size;
    buf = (char *)malloc (swap_area_size);
    hdp = (Lb_header_t  *)buf;
    if (buf == NULL)
	return (LB_MALLOC_FAILED);
    if (lseek (fd, 0, 0) < 0 ||
	MISC_read (fd, buf, swap_area_size) != swap_area_size) {
	free (buf);
	return (LB_LB_ERROR);
    }

    Byte_swap_control_area (buf, n_slots, taga_size);
    hdp->is_bigendian = 0xff;
    bad_be = -1;
    i_am_bigendian = MISC_i_am_bigendian ();
    if (lseek (fd, (char *)&(hdp->is_bigendian) - buf, 0) < 0 ||
	MISC_write (fd, &bad_be, sizeof (char)) != sizeof (char) ||
	lseek (fd, 0, 0) < 0 ||
	MISC_write (fd, buf, swap_area_size) != swap_area_size ||
	lseek (fd, (char *)&(hdp->is_bigendian) - buf, 0) < 0 ||
	MISC_write (fd, &i_am_bigendian, sizeof (char)) != sizeof (char)) {
	free (buf);
	return (LB_LB_ERROR);
    }
    free (buf);
    MISC_close (fd);

    return (LB_SUCCESS);
}

/********************************************************************
			
    Description: This function byte swaps all fields in the LB header. 

    Input/Output:	buf - pointer to the LB header.

********************************************************************/

static void Byte_swap_header (char *buf)
{
    Lb_header_t *hd;
    lb_t *pt;
    int i;

    hd = (Lb_header_t *)buf;
    if (hd->is_bigendian == MISC_i_am_bigendian ())
	return;

    pt = &(hd->lb_id);
    for (i = 0; i < 14; i++) {		/* we have 14 lb_t fields */
	*pt = LB_byte_swap (*pt);
	pt++;
    }
    hd->sms_ok = 0;
}
	
/********************************************************************
			
    Description: This function byte swaps the following fields in 
		the LB control area: header, dir and msginfo.

    Input:	buf - pointer to the LB control area.
		n_slots - number of dir slots.
		taga_size - size of the tag area.

********************************************************************/

static void Byte_swap_control_area (char *buf, int n_slots, int taga_size)
{
    Lb_header_t *hd;
    LB_dir *lb_dir;
    unsigned int num_ptr;
    int ptr, nmsgs;
    int ppt;
    int msginfo_off, i;

    hd = (Lb_header_t *)buf;
    Byte_swap_header (buf);

    num_ptr = hd->num_pt;
    nmsgs = GET_NMSG (num_ptr);
    ptr = GET_POINTER(num_ptr);		/* point to the new message */

    ppt = ptr - nmsgs;			/* point to the first interested msg */
    if (ppt < 0)
	ppt += n_slots;

    lb_dir = (LB_dir *)(buf + sizeof (Lb_header_t));
    msginfo_off = sizeof (Lb_header_t) + n_slots * sizeof (LB_dir) + taga_size;
    for (i = 0; i < n_slots; i++) { 	/* swap the entire msg directory */
	lb_dir[i].id = LB_byte_swap (lb_dir[i].id);
	lb_dir[i].loc = LB_byte_swap (lb_dir[i].loc);
    }

    if (hd->lb_types & LB_DB) {	/* swap the msginfo area */
	LB_msg_info_t *msginfo;
	int ind;

	msginfo = (LB_msg_info_t *)(buf + msginfo_off);
	for (i = 0; i < nmsgs; i++) {
	    ind = (ppt + i) % n_slots;
	    ind = DB_WORD_OFFSET (ind, lb_dir[ind].loc);
	    msginfo[ind].len = LB_byte_swap (msginfo[ind].len);
	    msginfo[ind].loc = LB_byte_swap (msginfo[ind].loc);
	}
    }
    else if (hd->msg_size == 0) {
	LB_msg_info_seq_t *msginfo;
	int ind;

	msginfo = (LB_msg_info_seq_t *)(buf + msginfo_off);
	for (i = 0; i < nmsgs; i++) {
	    ind = (ppt + i) % n_slots;
	    msginfo[ind].len = LB_byte_swap (msginfo[ind].len);
	}
    }
}

static unsigned int Randomize (unsigned int in) {

    /* if the following funcitons are not available, a direct implementation
       is: Xn+1 = (aXn + c) mod m, where n >= 0, m = 2^48, a = 0x5DEECE66D
       and c = 0xB */
    srand48 (in);
    return (lrand48 ());
}
		
/********************************************************************
			
    Returns a key associated with file "pathname" for shared memory.
    Returns -1 if the file does not exist.

********************************************************************/

static key_t my_ftok (const char *pathname) {
    unsigned int key_value, shift;
    struct stat buf;

    if (stat (pathname, &buf ) < 0)	/* Get file stats */
	return (-1);

    key_value = Randomize (buf.st_ino & 0xffffffff);
    shift = 32;
    if (sizeof (buf.st_ino) > 4)
	key_value += Randomize (Randomize (buf.st_ino >> shift));
    key_value = key_value & 0x7fffffff;
    if (key_value == 0)
	key_value = 198353843;		/* a randomly picked number */

    return ((key_t)key_value);
} 

#ifdef LB_THREADED 

/********************************************************************

    Description: This function initializes thread specific data key 
		for the per thread data structure. Because it is 
		difficult to return to the caller, the function 
		terminates the application in certain error contitions.

*********************************************************************/

static void Key_init_func (void)
{

    if (pthread_key_create (&Ptd_key, Thread_cleanup) != 0)
	Key_create_failed = 1;
    else 
	Key_create_failed = 0;
}

/********************************************************************
			
    Description: This function is called before a thread terminates. 
		It remove all lock records generated by this thread
		and closes all LBs opened by this thread.

    Input:	arg - argument - not used.

********************************************************************/

static void Thread_cleanup (void *arg)
{
    int index;
    pthread_t thread_id;

    pthread_mutex_lock (&Lb_open_mutex);
    thread_id = pthread_self ();
    for (index = 0; index < Lb_array_size; index++) {
	LB_struct *lb;

	if (Lb_handle[index] == NULL)
	    continue;

	lb = Lb_handle[index];

	/* close this LB */
	if (lb->thread_id == thread_id) {
	    EN_close_notify (index + MIN_INDEX);	
	    LB_cleanup_locks (lb);
	    Free_lb_resources (lb);
	}
    }
    pthread_mutex_unlock (&Lb_open_mutex);
}
#endif

/********************************************************************
			
    Description: This is a service function that prints LB compiler
		flags used.

********************************************************************/


void LB_test_compiler_flag (void)
{
#ifdef LB_THREADED
    printf ("LB_THREADED defined\n");
#endif
#ifdef LB_NTF_SERVICE
    printf ("LB_NTF_SERVICE defined\n");
#endif
#ifdef MMAP_UNMAP_NEEDED
    printf ("MMAP_UNMAP_NEEDED defined\n");
#endif
#ifdef LINUX
    printf ("LINUX defined\n");
#endif
#ifdef SUNOS
    printf ("SUNOS defined\n");
#endif
#ifdef USE_MEMORY_CHECKER
    printf ("USE_MEMORY_CHECKER defined\n");
#endif
}


