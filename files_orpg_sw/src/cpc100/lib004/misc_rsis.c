/**********************************************************************

    Description: This is the Record Storage and Indexing Service (RSIS) 
		library.

**********************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/09/14 15:41:50 $
 * $Id: misc_rsis.c,v 1.20 2005/09/14 15:41:50 jing Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <misc_rsis.h>
#include <misc.h>

typedef unsigned short Rbt_ind_t;
				/* red-black tree index type */

typedef int FIM_TYPE;		/* type used for free index management */
#define FIM_SIZE 4		/* number of bytes of FIM_TYPE */
#define FIM_NBITS 32		/* number of bits of FIM_TYPE */

typedef struct nodeTag {	/* structure for a red-black tree node */
    Rbt_ind_t left;      	/* left child */
    Rbt_ind_t right;      	/* right child */
    Rbt_ind_t parent;    	/* parent */
    Rbt_ind_t color;     	/* node color (Rbt_BLACK, Rbt_RED) */
} Rbt_node_t;

typedef struct {		/* header structure of the RSIS area */
    int maxn;			/* maximum number of records */
    int n_keys;			/* number of search keys */
    int rec_size;		/* user record size */
    int hd_size;		/* RSIS header size */
    int bitmap_size;		/* bitmap size in number of FIM_TYPE */
    int frind;			/* current free index offset */
    int nrecs;			/* current number of records */
    int root[1];		/* current root node index */
} Rsis_header_t;

typedef struct {		/* local RSIS structure */
    unsigned int check_sum;	/* check sum value */
    int which_key;		/* save which_key argument for next traverse */
    int ind;			/* save node index for next traverse */
    Rsis_header_t *hd;		/* pointer to the RSIS header */
    int (*compare)(int, void *, void *);
				/* user's comparison function */
    FIM_TYPE *bitmap;		/* pointer to the free index bitmap array */
    char *recs;			/* pointer to the user record array */
    Rbt_node_t *node[1];	/* pointer ot the node array */
} Rsis_local_t;

/* Red-Black tree description */
enum { Rbt_BLACK, Rbt_RED };

#define NON_IND 0xffff
#define NIL_IND 0

/* the following variables are designed for simplifying use of the
   red-black tree routines */
static Rbt_ind_t root;
static Rbt_node_t *node;
static char *Recs;
static int Rec_size;
static int (*Compare)(int, void *, void *);


#define Test_index(a,b) Test_and_free_index (0, a, b)
#define Free_index(a,b) Test_and_free_index (1, a, b)

static int insert (Rsis_header_t *hd, int new_ind, int which_key);
static int Get_free_index (Rsis_local_t *rsis);
static int deleteNode (Rbt_ind_t z);
static Rbt_ind_t findNode (int which_key, void *key);
static void Set_local_vars (Rsis_local_t *rsis, int which_key);
static void rotateLeft(Rbt_ind_t x);
static void rotateRight(Rbt_ind_t x);
static void insertFixup(Rbt_ind_t x);
static void deleteFixup(Rbt_ind_t x);
static Rbt_ind_t findRightNeighbor (Rbt_ind_t c);
static Rbt_ind_t findLeftNeighbor (Rbt_ind_t c);
static int Test_and_free_index (int free, Rsis_local_t *rsis, int ind);
static Rsis_local_t *Init_rsis_local (Rsis_header_t *hd, 
			char *lbuf, int (*compare)(int, void *, void *));
static unsigned int Check_sum (Rsis_local_t *rsis);


/************************************************************************

    Description: returns the buffer size needed for managing "maxn"
		records with "n_keys" keys.

    Input:	maxn - maximum number of records allowed.
		n_keys - number of keys.
		rec_size - size of user records.

    Return:	returns the buffer size needed.

    Note:	A bug was found. In order to keep backward compatibily
		I use negative "maxn" for the old version. This way to
		fix the error will minimize files to be changed. The 
		old_version support will be phased out in the future.

************************************************************************/

int RSIS_size (int maxn, int n_keys, int rec_size)
{
    int size, old_version;

    if (maxn < 0) {
	maxn = -maxn;
	old_version = 1;
    }
    else
	old_version = 0;

    if (maxn <= 0 || maxn >= NON_IND - 1 || n_keys <= 0 || rec_size < 0)
	return (RSIS_INVALID_ARGS);

    maxn++;			/* allocate one more record since the rbtree 
				   uses the first index for NIL node */
    size = sizeof (Rsis_header_t) + (n_keys - 1) * sizeof (int) +
		maxn * rec_size + n_keys * maxn * sizeof (Rbt_node_t);
    if (old_version)
	size += ((maxn + FIM_SIZE - 1) / FIM_SIZE) * FIM_SIZE;
    else
	size += ((maxn + FIM_NBITS - 1) / FIM_NBITS) * FIM_SIZE;
    return (size);
}

/************************************************************************

    Description: Initializes the RSIS data structure and the RSIS local
		structure.

    Input:	maxn - maximum number of records allowed.
		n_keys - number of keys.
		rec_size - size of user records.
		buf - pointer to a buffer used by the RSIS. If NULL, the
		     buffer is allocate and reallocated by this module.
		lbuf - local buffer area.
		compare - user provided comparison function.

    Return:	The pointer to the local RSIS struct (used as access ID)
		on success or NULL on failure.

************************************************************************/

char *RSIS_init (int maxn, int n_keys, int rec_size, 
		char *buf, char *lbuf, int (*compare)(int, void *, void *))
{
    Rsis_local_t *rsis;
    Rsis_header_t *hd;
    int size, i;

    if ((size = RSIS_size (maxn, n_keys, rec_size)) < 0)
	return (NULL);			/* check arguments */
    if (buf == NULL)
	hd = (Rsis_header_t *)MISC_malloc (size);
    else
	hd = (Rsis_header_t *)buf;

    /* initialize the header */
    maxn++;				/* see RSIS_size */
    hd->maxn = maxn;    
    hd->n_keys = n_keys;    
    hd->rec_size = rec_size;    
    hd->hd_size = sizeof (Rsis_header_t) + (n_keys - 1) * sizeof (int);    
    hd->bitmap_size = (maxn + FIM_NBITS - 1) / FIM_NBITS;    
    hd->frind = hd->nrecs = 0;

    if ((rsis = Init_rsis_local (hd, lbuf, compare)) == NULL)
	return (NULL);				/* set RSIS local struct */

    for (i = 0; i < hd->bitmap_size; i++)	/* init bit map */
	rsis->bitmap[i] = 0;
    Get_free_index (rsis);		/* reserve index NIL_IND */

    /* initialize rbtree and NIL nodes */
    for (i = 0; i < n_keys; i++) {
	Rbt_node_t *n;

	hd->root[i] = NIL_IND;
	n = rsis->node[i] + NIL_IND;
	n->left = NIL_IND;
	n->right = NIL_IND;
	n->parent = NON_IND;
	n->color = Rbt_BLACK;
    }

    return ((char *)rsis);
}

/************************************************************************

    Description: Initializes the RSIS local struct using existing RSIS
		data structure.

    Input:	buf - pointer to the existing RSIS data struct.
		lbuf - local buffer area.
		compare - the user's comparison function.

    Return:	The pointer to the local RSIS struct (used as access ID)
		on success or NULL on failure.

************************************************************************/

char *RSIS_localize (char *buf, char *lbuf, 
				int (*compare)(int, void *, void *))
{

    return ((char *)Init_rsis_local ((Rsis_header_t *)buf, lbuf, compare));
}

/************************************************************************

    Returns the size of the buffer for Init_rsis_local.

    Input:	n_keys - number of keys.
		
************************************************************************/

int RSIS_local_buf_size (int n_keys) {
    return (sizeof (Rsis_local_t) + 
			(n_keys - 1) * sizeof (Rbt_node_t *));
}

/************************************************************************

    Description: Initializes the RSIS local struct.

    Input:	hd - pointer to the RSIS header.
		lbuf - local buffer area.
		compare - the user's comparison function.
		
************************************************************************/

static Rsis_local_t *Init_rsis_local (Rsis_header_t *hd, 
			char *lbuf, int (*compare)(int, void *, void *))
{
    Rsis_local_t *rsis;
    char *pt;
    int i;

    if (lbuf == NULL)
	rsis = (Rsis_local_t *)MISC_malloc (sizeof (Rsis_local_t) + 
			(hd->n_keys - 1) * sizeof (Rbt_node_t *));
    else
	rsis = (Rsis_local_t *)lbuf;

    pt = (char *)hd;
    rsis->hd = hd;
    pt += hd->hd_size;
    rsis->bitmap = (FIM_TYPE *)pt;
    pt += hd->bitmap_size * FIM_SIZE;  
    rsis->recs = pt;
    pt += hd->maxn * hd->rec_size;
    for (i = 0; i < hd->n_keys; i++) {
	rsis->node[i] = (Rbt_node_t *)pt;
	pt += hd->maxn * sizeof (Rbt_node_t);
    }
    rsis->compare = compare;
    rsis->which_key = 0;    
    rsis->ind = NON_IND;    
    rsis->check_sum = Check_sum (rsis);

    return (rsis);
}

/************************************************************************

    Description: Verifies if the RSIS local struct is corrupted.

    Input:	rsid - the RSIS local struct (RSIS ID).

    Return:	non-zero if it is corrupted or 0 otherwise.

************************************************************************/

int RSIS_is_corrupted (char *rsid)
{
    Rsis_local_t *rsis;

    rsis = (Rsis_local_t *)rsid;
    if (rsis->check_sum != Check_sum (rsis))
	return (1);
    else
	return (0);
}

/************************************************************************

    Description: Inserts a new record "new_entry" in the RSIS "rsid" and 
		updates the indices.

    Input:	rsid - the RSIS local struct (RSIS ID).
		new_rec - the new record to be inserted.

    Return:	returns the record index on success or a negetive error
		number of failure.

************************************************************************/

int RSIS_insert (char *rsid, void *new_rec)
{
    Rsis_local_t *rsis;
    Rsis_header_t *hd;
    int ind, i;

    rsis = (Rsis_local_t *)rsid;
    hd = rsis->hd;
    ind = Get_free_index (rsis);
    if (ind < 0)
	return (ind);

    Set_local_vars (rsis, -1);
    memcpy (rsis->recs + ind * Rec_size, (char *)new_rec, Rec_size);
    for (i = 0; i < hd->n_keys; i++) {
	node = rsis->node[i];
	root = hd->root[i];
	insert (hd, ind, i);
	hd->root[i] = root;
    }
    return (ind);
}

/************************************************************************

    Description: Deletes the record of index "ind" in the RSIS "rsid" and 
		updates the indices.

    Input:	rsid - the RSIS local struct (RSIS ID).
		ind - the index of the record to be deleted.

    Return:	returns 0 on success or a negetive error
		number of failure.

************************************************************************/

int RSIS_delete (char *rsid, int ind)
{
    Rsis_local_t *rsis;
    Rsis_header_t *hd;
    int i, ret;

    rsis = (Rsis_local_t *)rsid;
    hd = rsis->hd;
    ret = Test_index (rsis, ind);
    if (ret < 0)
	return (ret);

    Set_local_vars (rsis, -1);
    for (i = 0; i < hd->n_keys; i++) {
	node = rsis->node[i];
	root = hd->root[i];
	deleteNode ((Rbt_ind_t)ind);
	hd->root[i] = root;
    }
    Free_index (rsis, ind);
    return (0);
}

/************************************************************************

    Description: finds a record by specified key. If there are records
		with the same key, one of the records is returned. If
		the key is not found, a neighboring record is returned.

    Input:	rsid - the RSIS local struct (RSIS ID).
		which_key - which key used in search.
		key - The user struct containing the key for search.

    Output:	record - returns the user record found.

    Return:	returns a record index on success or RSIS_NOT_FOUND
		if there no record at all.

************************************************************************/

int RSIS_find (char *rsid, int which_key, void *key, void *record)
{
    Rsis_local_t *rsis;
    Rbt_ind_t ind;

    rsis = (Rsis_local_t *)rsid;
    if (which_key < 0 || which_key >= rsis->hd->n_keys)
	return (RSIS_NOT_FOUND);
    Set_local_vars (rsis, which_key);

    ind = findNode (which_key, key);
    rsis->which_key = which_key;
    rsis->ind = ind;
    if (ind == NIL_IND)
	return (RSIS_NOT_FOUND);
    *((char **)record) = Recs + ind * Rec_size;
    return (ind);
}

/************************************************************************

    Description: Finds a neighboring record of a given record.

    Input:	rsid - the RSIS local struct (RSIS ID).
		which_key - the key involved.
		dir - direction: RSIS_LEFT, or RSIS_RIGHT.
		ind - the index of the record to start with.
		
    Output:	record - returns the user record found.

    Return:	returns a record index on success or RSIS_NOT_FOUND
		if there is no requested neighbor.

************************************************************************/

int RSIS_traverse (char *rsid, int which_key, int dir, int ind, void *record)
{
    Rsis_local_t *rsis;
    Rbt_ind_t i;
    int ret;

    rsis = (Rsis_local_t *)rsid;
    if (which_key < 0 || which_key >= rsis->hd->n_keys)
	return (RSIS_NOT_FOUND);
    ret = Test_index (rsis, ind);
    if (ret < 0)
	return (ret);
    Set_local_vars (rsis, which_key);

    if (dir == RSIS_LEFT)
	i = findLeftNeighbor ((Rbt_ind_t)ind);
    else
	i = findRightNeighbor ((Rbt_ind_t)ind);

    rsis->which_key = which_key;
    rsis->ind = i;
    if (i == NIL_IND)
	return (RSIS_NOT_FOUND);
    *((char **)record) = Recs + i * Rec_size;
    return (i);
}

/************************************************************************

    Description: Finds the left neighbor of the previous record.

    Input:	rsid - the RSIS local struct (RSIS ID).
		
    Output:	record - returns the user record found.

    Return:	returns a record index on success or RSIS_NOT_FOUND
		if there is no requested neighbor.

************************************************************************/

int RSIS_left (char *rsid, void *record)
{
    Rsis_local_t *rsis;

    rsis = (Rsis_local_t *)rsid;
    return (RSIS_traverse (rsid, rsis->which_key, 
					RSIS_LEFT, rsis->ind, record));
}

/************************************************************************

    Description: Finds the right neighbor of the previous record.

    Input:	rsid - the RSIS local struct (RSIS ID).
		
    Output:	record - returns the user record found.

    Return:	returns a record index on success or RSIS_NOT_FOUND
		if there is no requested neighbor.

************************************************************************/

int RSIS_right (char *rsid, void *record)
{
    Rsis_local_t *rsis;

    rsis = (Rsis_local_t *)rsid;
    return (RSIS_traverse (rsid, rsis->which_key, 
					RSIS_RIGHT, rsis->ind, record));
}

/************************************************************************

    Description: This returns the pointer to the beginning of the
		record area.

    Input:	rsid - the RSIS local struct (RSIS ID).

    Return:	returns the pointer.

************************************************************************/

char *RSIS_get_record_address (char *rsid)
{

    return (((Rsis_local_t *)rsid)->recs);
}

/************************************************************************

    Description: This returns the record index next to "ind".

    Input:	rsid - the RSIS local struct (RSIS ID).
		ind - the current record index.

    Return:	returns the next record index on success or a negetive error
		number of failure.

************************************************************************/

int RSIS_get_next_ind (char *rsid, int ind)
{
    Rsis_local_t *rsis;
    Rsis_header_t *hd;
    int i;

    rsis = (Rsis_local_t *)rsid;
    hd = rsis->hd;
    for (i = ind + 1; i < hd->maxn; i++) {
	if (Test_index (rsis, i) == 0)
	    return (i);
    }
    return (RSIS_NOT_FOUND);
}

/************************************************************************

    Description: Sets the local variables used by the red-black tree
		routines.

    Input:	rsis - the RSIS local struct.
		which_key - the key involved. If it < 0, node and root
		is not set.
		
************************************************************************/

static void Set_local_vars (Rsis_local_t *rsis, int which_key)
{
    Rsis_header_t *hd;

    hd = rsis->hd;
    Recs = rsis->recs;
    Compare = rsis->compare;
    Rec_size = hd->rec_size;
    if (which_key >= 0) {
	node = rsis->node[which_key];
	root = hd->root[which_key];
    }
    return;
}

/************************************************************************

    Description: Gets a free node index. This one of the free node 
		(memory) management routines. We try to reuse the nodes
		in the first part of the buffer to reduce the number
		of memory pages.

    Input:	rsis - the RSIS local struct.

    Return:	the free index on success or a negative error number.
		
************************************************************************/

static int Get_free_index (Rsis_local_t *rsis)
{
    Rsis_header_t *hd;
    int nind, stbyte, stb, reachend, stind, maxn, ss;

    hd = rsis->hd;
    stind = hd->frind;
    maxn = hd->maxn;
    ss = hd->nrecs * 12 / 10 + 2;	/* current search size */
    if (ss > maxn)
	ss = maxn;
    nind = stind;
    stbyte = (nind >> 3) % FIM_SIZE;	/* starting byte index */
    stb = nind % 8;		/* starting bit index */
    reachend = 0;
    while (1) {
	FIM_TYPE *cw;		/* current word */
	int byte, b;

	if (nind >= ss) {
	    if (!reachend)
		nind = stbyte = stb = 0;
	    reachend = 1;
	}
	if (reachend && nind > stind)	/* not found */
	    return (RSIS_TOO_MANY_RECORDS);
	cw = rsis->bitmap + nind / FIM_NBITS;
	if (stbyte == 0 && stb == 0 && ~(*cw) == 0) {
	    nind += FIM_NBITS;
	    continue;
	}
	for (byte = stbyte; byte < FIM_SIZE; byte++) {
	    char cb;

	    cb = *((char *)cw + byte);
	    if (stb == 0 && ~cb == 0) {
		nind += 8;
		continue;
	    }
	    for (b = stb; b < 8; b++) {
		if (nind < maxn && (cb & (1 << b)) == 0) {	/* found */
		    *((char *)cw + byte) |= 1 << b;
		    hd->frind = nind;
		    hd->nrecs++;
		    return (nind);
		}
		nind++;
	    }
	    stb = 0;
	}
	stbyte = 0;
    }
}

/************************************************************************

    Description: Checks whether node index "ind" is valid. It also
		frees the node index "ind" if "free" is non-zero.

    Input:	free - flag indicating free is needed.
		rsis - the RSIS local struct.
		ind - node index to be checked and freed.

    Return:	zero if "ind" is valid, a negative error number otherwise.
		
************************************************************************/

static int Test_and_free_index (int free, Rsis_local_t *rsis, int ind)
{
    Rsis_header_t *hd;
    FIM_TYPE *word;			/* current word */
    int byte, bit;

    hd = rsis->hd;
    if (ind <= 0 || ind >= hd->maxn)	/* index 0 is reserved for NIL */
	return (RSIS_INVALID_INDEX);

    byte = (ind >> 3) % FIM_SIZE;	/* byte index */
    bit = ind % 8;			/* bit index */
    word = rsis->bitmap + ind / FIM_NBITS;
    if (*((char *)word + byte) & (1 << bit)) {
	if (free) {
	    *((char *)word + byte) &= ~(1 << bit);
	    hd->nrecs--;
	}
	return (0);
    }
    else
	return (RSIS_INVALID_INDEX);
}

/************************************************************************

    Description: returns the check sum of the RSIS local struct.

    Input:	rsis - poiner to the RSIS local struct.

    Return:	returns the check sum.

************************************************************************/

static unsigned int Check_sum (Rsis_local_t *rsis)
{
    unsigned int sum, *pt, *end;

    pt = (unsigned int *)rsis + 3;
    sum = 0;
    end = pt + (4 + rsis->hd->n_keys);
    while (pt < end) {
	sum += *pt;
	pt++;
    }
    return (sum);
}


/*************************************************************************

    Description: The following are the Red-black tree code I adopted from
		http://members.xoom.com/thomasn/s_man.htm. I added the
		traverse functions. I turned the code into an index based
		tree instead of the original pointer based. I also modify
		the code such that can support node deletion based on 
		index instead of on key. This is required for building
		multiple associated trees. I modified the tree such that
		it will support duplicated keys.

*************************************************************************/

static void rotateLeft(Rbt_ind_t x) {

   /**************************
    *  rotate node x to left *
    **************************/
    Rbt_node_t *nx, *ny;
    Rbt_ind_t y;

    nx = node + x;
    y = nx->right;
    ny = node + y;

    /* establish nx->right link */
    nx->right = ny->left;
    if (ny->left != NIL_IND) node[ny->left].parent = x;

    /* establish ny->parent link */
    if (y != NIL_IND) ny->parent = nx->parent;
    if (nx->parent != NON_IND) {
        if (x == node[nx->parent].left)
            node[nx->parent].left = y;
        else
            node[nx->parent].right = y;
    } else {
        root = y;
    }

    /* link x and y */
    ny->left = x;
    if (x != NIL_IND) nx->parent = y;
}

static void rotateRight(Rbt_ind_t x) {

   /****************************
    *  rotate node x to right  *
    ****************************/
    Rbt_node_t *nx, *ny;
    Rbt_ind_t y;

    nx = node + x;
    y = nx->left;
    ny = node + y;

    /* establish nx->left link */
    nx->left = ny->right;
    if (ny->right != NIL_IND) node[ny->right].parent = x;

    /* establish ny->parent link */
    if (y != NIL_IND) ny->parent = nx->parent;
    if (nx->parent != NON_IND) {
        if (x == node[nx->parent].right)
            node[nx->parent].right = y;
        else
            node[nx->parent].left = y;
    } else {
        root = y;
    }

    /* link x and y */
    ny->right = x;
    if (x != NIL_IND) nx->parent = y;
}

static void insertFixup(Rbt_ind_t x) {

   /*************************************
    *  maintain Red-Black tree balance  *
    *  after inserting node x           *
    *************************************/
    Rbt_node_t *tn;

    /* check Red-Black properties */
    while (x != root && node[node[x].parent].color == Rbt_RED) {
        /* we have a violation */
	tn = node + node[x].parent;
        if (node[x].parent == node[tn->parent].left) {
            Rbt_ind_t y = node[tn->parent].right;
            if (node[y].color == Rbt_RED) {

                /* uncle is Rbt_RED */
                tn->color = Rbt_BLACK;
                node[y].color = Rbt_BLACK;
                node[tn->parent].color = Rbt_RED;
                x = tn->parent;
            } else {

                /* uncle is Rbt_BLACK */
                if (x == tn->right) {
                    /* make x a left child */
                    x = node[x].parent;
                    rotateLeft(x);
                }

                /* recolor and rotate */
		tn = node + node[x].parent;
                tn->color = Rbt_BLACK;
                node[tn->parent].color = Rbt_RED;
                rotateRight(tn->parent);
            }
        } else {

            /* mirror image of above code */
            Rbt_ind_t y = node[tn->parent].left;
            if (node[y].color == Rbt_RED) {

                /* uncle is Rbt_RED */
                tn->color = Rbt_BLACK;
                node[y].color = Rbt_BLACK;
                node[tn->parent].color = Rbt_RED;
                x = tn->parent;
            } else {

                /* uncle is Rbt_BLACK */
                if (x == tn->left) {
                    x = node[x].parent;
                    rotateRight(x);
                }
		tn = node + node[x].parent;
                tn->color = Rbt_BLACK;
                node[tn->parent].color = Rbt_RED;
                rotateLeft(tn->parent);
            }
        }
    }
    node[root].color = Rbt_BLACK;
}

static int insert (Rsis_header_t *hd, int new_ind, int which_key) {
    Rbt_ind_t current, parent, x;
    Rbt_node_t *tn;
    char *new_key;

   /***********************************************
    *  allocate node for data and insert in tree  *
    ***********************************************/

    new_key = Recs + new_ind * Rec_size;

    /* find future parent */
    current = root;
    parent = NON_IND;
    while (current != NIL_IND) {
	int ret;

	tn = node + current;
        ret = Compare (which_key, new_key, Recs + current * Rec_size);
        parent = current;
        current = (ret == RSIS_LESS) ?tn->left : tn->right;
    }

    /* setup new node */
    x = new_ind;
    tn = node + x;
    tn->parent = parent;
    tn->left = NIL_IND;
    tn->right = NIL_IND;
    tn->color = Rbt_RED;

    /* insert node in tree */
    if(parent != NON_IND) {
	tn = node + parent;
        if(Compare (which_key, new_key, Recs + parent * Rec_size) == RSIS_LESS)
            tn->left = x;
        else
            tn->right = x;
    } else {
        root = x;
    }

    insertFixup(x);

    return (x);
}

static void deleteFixup(Rbt_ind_t x) {

   /*************************************
    *  maintain Red-Black tree balance  *
    *  after deleting node x            *
    *************************************/
    Rbt_node_t *tn, *wn;

    while (x != root && node[x].color == Rbt_BLACK) {
	tn = node + node[x].parent;
        if (x == tn->left) {
            Rbt_ind_t w = tn->right;
	    wn = node + w;
            if (wn->color == Rbt_RED) {
                wn->color = Rbt_BLACK;
                tn->color = Rbt_RED;
                rotateLeft (node[x].parent);
                w = tn->right;
	        wn = node + w;
            }
            if (node[wn->left].color == Rbt_BLACK && 
				node[wn->right].color == Rbt_BLACK) {
                wn->color = Rbt_RED;
                x = node[x].parent;
		tn = node + node[x].parent;
            } else {
                if (node[wn->right].color == Rbt_BLACK) {
                    node[wn->left].color = Rbt_BLACK;
                    wn->color = Rbt_RED;
                    rotateRight (w);
                    w = tn->right;
	            wn = node + w;
                }
                wn->color = tn->color;
                tn->color = Rbt_BLACK;
                node[wn->right].color = Rbt_BLACK;
                rotateLeft (node[x].parent);
                x = root;
		tn = node + node[x].parent;
            }
        } else {
            Rbt_ind_t w = tn->left;
	    wn = node + w;
            if (wn->color == Rbt_RED) {
                wn->color = Rbt_BLACK;
                tn->color = Rbt_RED;
                rotateRight (node[x].parent);
                w = tn->left;
	        wn = node + w;
            }
            if (node[wn->right].color == Rbt_BLACK && 
				node[wn->left].color == Rbt_BLACK) {
                wn->color = Rbt_RED;
                x = node[x].parent;
		tn = node + node[x].parent;
            } else {
                if (node[wn->left].color == Rbt_BLACK) {
                    node[wn->right].color = Rbt_BLACK;
                    wn->color = Rbt_RED;
                    rotateLeft (w);
                    w = tn->left;
	            wn = node + w;
                }
                wn->color = tn->color;
                tn->color = Rbt_BLACK;
                node[wn->left].color = Rbt_BLACK;
                rotateRight (node[x].parent);
                x = root;
		tn = node + node[x].parent;
            }
        }
    }
    node[x].color = Rbt_BLACK;
}

static int deleteNode (Rbt_ind_t z) {
    Rbt_ind_t x, y, np;

   /*****************************
    *  delete node z from tree  *
    *****************************/

    if (node[z].left == NIL_IND || node[z].right == NIL_IND) {
        /* y has a NIL_IND node as a child */
        y = z;
    } else {
        /* find tree successor with a NIL_IND node as a child */
        y = node[z].right;
        while (node[y].left != NIL_IND) y = node[y].left;
    }

    /* x is y's only child */
    if (node[y].left != NIL_IND)
        x = node[y].left;
    else
        x = node[y].right;

    /* remove y from the parent chain */
    np = node[y].parent;
    node[x].parent = np;
    if (np != NON_IND) {
        if (y == node[np].left)
            node[np].left = x;
        else
            node[np].right = x;
    }
    else
        root = x;

    if (node[y].color == Rbt_BLACK)
        deleteFixup (x);

    if (y != z) {		/* replace index z by y so we can free z */

	if (root == z)
	    root = y;
	if (node[z].left != NIL_IND)
	    node[node[z].left].parent = y;
	if (node[z].right != NIL_IND)
	    node[node[z].right].parent = y;
	np = node[z].parent;
	if (np != NON_IND) {
	    if (z == node[np].left)
		node[np].left = y;
	    else
		node[np].right = y;
	}
	memcpy (node + y, node + z, sizeof (Rbt_node_t));
    }

    return (0);
}

static Rbt_ind_t findRightNeighbor (Rbt_ind_t c)
{
    Rbt_ind_t cur;

    cur = c;
    if (node[cur].right != NIL_IND) {
	cur = node[cur].right;
	while (node[cur].left != NIL_IND)
	    cur = node[cur].left;
    }
    else {
	Rbt_ind_t y = node[cur].parent;
	if (y == NON_IND)
	    return (NIL_IND);
	while (cur == node[y].right) {
	    cur = y;
	    y = node[y].parent;
	    if (y == NON_IND)
		return (NIL_IND);
	}
	if (node[cur].right != y)
	    cur = y;
    }
    return (cur);
}

static Rbt_ind_t findLeftNeighbor (Rbt_ind_t c)
{
    Rbt_ind_t cur;

    cur = c;
    if (node[cur].left != NIL_IND) {
	cur = node[cur].left;
	while (node[cur].right != NIL_IND)
	    cur = node[cur].right;
    }
    else {
	Rbt_ind_t y = node[cur].parent;
	if (y == NON_IND)
	    return (NIL_IND);
	while (cur == node[y].left) {
	    cur = y;
	    y = node[y].parent;
	    if (y == NON_IND)
		return (NIL_IND);
	}
	if (node[cur].left != y)
	    cur = y;
    }
    return (cur);
}

static Rbt_ind_t findNode (int which_key, void *key) {

   /*******************************
    *  find node by key           *
    *******************************/
    Rbt_node_t *tn;

    Rbt_ind_t parent = NIL_IND;
    Rbt_ind_t current = root;
    while(current != NIL_IND) {
	int ret;

	tn = node + current;
	ret = Compare (which_key, key, Recs + current * Rec_size);
        if(ret == RSIS_EQUAL) {
            return current;
        } else {
	    parent = current;
            current = (ret == RSIS_LESS) ? tn->left : tn->right;
        }
    }
    return parent;
}
