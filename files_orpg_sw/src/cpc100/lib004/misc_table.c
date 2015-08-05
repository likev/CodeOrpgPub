
/*************************************************************
			
    Description: This file implements a variable table manager.

**************************************************************/
/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2009/03/05 22:04:19 $
 * $Id: misc_table.c,v 1.24 2009/03/05 22:04:19 jing Exp $
 * $Revision: 1.24 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include <misc.h>
#include <en.h>

typedef struct {			/* structure for the MISC table
					   module */
    int size;				/* size of the table buffer */
    short inc;				/* incremental size for remalloc */
    short entry_size;			/* size of table entry */
    short n_entries;			/* number of entries in the table */
    short keep_order;			/* boolean: whether or not entry order 
					   is preserved after deletion */
    char *table;			/* pointer to the table */
    int *n_ent_pt;			/* user registered pointer to a local
					   n_entries */
    char **tbl_pt;			/* user registered pointer to a local
					   table */
} Misc_table;

static int Malloc_retry = 0;		/* MISC_malloc retry control flag */

/*************************************************************
			
    Description: This function creates a new table. This is the
		older creation function.

    Input:	entry_size - table entry size in number of bytes;
		inc - incremental size, in number of entries, 
			when reallocating buffer.

    Return:	A table identifier, the pointer to the table 
		structure, for later access on success or
		NULL if malloc failed.

**************************************************************/

void *MISC_create_table (int entry_size, int inc)
{

    return (MISC_open_table (entry_size, inc, 1, NULL, NULL));
}

/*************************************************************
			
    Description: This function opens a new table. The initial
		table buffer size is "inc" entries.

    Input:	entry_size - table entry size in number of bytes;
		inc - incremental size, in number of entries, 
			when reallocating buffer.
		keep_order - If true, the table entry order is 
			preserved when an entry is deleted.
		n_ent_pt - address of a user variable for n_entries.
		tbl_pt - address of a user variable for tbl_pt.

    Return:	A table identifier, the pointer to the table 
		structure, for later access on success or
		NULL if malloc failed.

**************************************************************/

void *MISC_open_table (int entry_size, int inc, 
			int keep_order, int *n_ent_pt, char **tbl_pt)
{
    char *pt, *pt1;
    Misc_table *tbl;

    if (entry_size <= 0 || inc <= 0)
	return (NULL);

    pt = (char *)MISC_malloc (sizeof (Misc_table));
    pt1 = (char *)MISC_malloc (entry_size * inc);
    tbl = (Misc_table *)pt;
    tbl->size = inc;
    tbl->entry_size = entry_size;
    tbl->inc = inc;
    tbl->n_entries = 0;
    tbl->keep_order = keep_order;
    tbl->table = pt1;
    tbl->n_ent_pt = n_ent_pt;
    tbl->tbl_pt = tbl_pt;
    if (n_ent_pt != NULL)
	*n_ent_pt = 0;
    if (tbl_pt != NULL)
	*tbl_pt = pt1;
    return ((void *)pt);
}

/*************************************************************
			
    Description: This function frees memory segments allocated
		by the table identified by "tblpt".

    Input:	tblpt - table identifier.

**************************************************************/

void MISC_free_table (void *tblpt)
{
    Misc_table *tbl;

    tbl = (Misc_table *)tblpt;
    if (tbl->table != NULL)
	MISC_free (tbl->table);
    MISC_free (tblpt);
    return;
}

/*************************************************************
			
    Description: This function returns the pointer to the table
		identified by "tblpt".

    Input:	tblpt - table identifier.

    Output:	size - size of the table.

    Return:	pointer to the table or NULL on failure.

**************************************************************/

void *MISC_get_table (void *tblpt, int *size)
{
    Misc_table *tbl;

    if (tblpt == NULL) {
	*size = 0;
	return (NULL);
    }
    tbl = (Misc_table *)tblpt;
    *size = tbl->n_entries;
    return ((void *)tbl->table);
}

/*************************************************************
			
    Description: This function returns the pointer to a new 
		entry of table identified by "tblpt".

    Input:	tblpt - table identifier.

    Output:	ind - returns the index of the new entry in 
		the table if not NULL.

    Return:	pointer to the new entry or NULL on malloc failure.

**************************************************************/

void *MISC_table_new_entry (void *tblpt, int *ind)
{
    Misc_table *tbl;

    if (tblpt == NULL)
	return (NULL);
    tbl = (Misc_table *)tblpt;
    if (tbl->n_entries >= tbl->size) {
				/* we must reallocate more memory */
	int new_size;
	char *pt;

	new_size = (int)tbl->size + tbl->inc;
	if (new_size > 0x7fff) {	/* tbl->n_entries is a short */
	    MISC_log ("Table size too big\n");
	    return (NULL);
	}
	pt = (char *)MISC_malloc (tbl->entry_size * new_size);
	memcpy (pt, (char *)tbl->table, tbl->size * tbl->entry_size);
	MISC_free (tbl->table);
	tbl->table = pt;
	tbl->size = new_size;
	if (tbl->tbl_pt != NULL)
	    *(tbl->tbl_pt) = pt;
    }

    if (ind != NULL)
	*ind = tbl->n_entries;
    tbl->n_entries++;
    if (tbl->n_ent_pt != NULL)
	*(tbl->n_ent_pt) = tbl->n_entries;
    return ((void *)
	((char *)tbl->table + (tbl->n_entries - 1) * tbl->entry_size));
}

/*************************************************************
			
    Description: This function frees an entry pointered to
		by "entry" for the table identified by "tblpt".

    Input:	*tblpt - table identifier.
		ind - entry index to be freed in the table.

**************************************************************/

void MISC_table_free_entry (void *tblpt, int ind)
{
    Misc_table *tbl;
    char *ent;
    int good_size, len;

    if (tblpt == NULL)
	return;

    tbl = (Misc_table *)tblpt;
    if (ind >= tbl->n_entries)
	return;

    ent = tbl->table + ind * tbl->entry_size;
    if (tbl->keep_order) {
	len = (tbl->n_entries - ind - 1) * tbl->entry_size;
	if (len > 0)
	    memmove (ent, ent + tbl->entry_size, len);
    }
    else {
	char *last;

	last = tbl->table + (tbl->n_entries - 1) * tbl->entry_size;
	if (ent != last)
	    memcpy (ent, last, tbl->entry_size);
    }
    tbl->n_entries--;
    if (tbl->n_ent_pt != NULL)
	*(tbl->n_ent_pt) = tbl->n_entries;

    /* check whether we need to reduce the buffer */
    good_size = tbl->n_entries + 2 * tbl->inc;
    if (tbl->size > good_size) {
	char *pt;

	good_size -= tbl->inc;
	pt = (char *)MISC_malloc (good_size * tbl->entry_size);
	if (pt != NULL) {
	    memcpy (pt, (char *)tbl->table, tbl->n_entries * tbl->entry_size);
	    MISC_free (tbl->table);
	    tbl->table = pt;
	    tbl->size = good_size;
	    if (tbl->tbl_pt != NULL)
		*(tbl->tbl_pt) = pt;
	}
    }

    return;
}

/************************************************************************

    Searches for an entry in the table "tblpt" that equals (in terms of 
    "cmp") "ent". If found, returns 1 and "ind" set to the first found
    entry. Otherwise, returns 0 and "ind" set to the first entry that
    is greater than "ent". The table must be in non-decreasing order.

************************************************************************/

int MISC_table_search (void *tblpt, 
			void *ent, int (*cmp) (void *, void *), int *ind)
{
    Misc_table *tbl;

    tbl = (Misc_table *)tblpt;
    return (MISC_bsearch (ent, tbl->table, tbl->n_entries, 
					tbl->entry_size, cmp, ind));
}

/************************************************************************

    Searches for an entry in the table "tbl" of size "n" that equals 
    (in terms of "cmp") "ent". The size of each item in the table is 
    "item_size". If found, returns 1 and "ind" set to the first found
    entry. Otherwise, returns 0 and "ind" set to the first entry that
    is greater than "ent". The table must be in non-decreasing order.

************************************************************************/

int MISC_bsearch (void *ent, void *tbl, int tbl_size, int item_size, 
				int (*cmp) (void *, void *), int *ind) {
    int st, end;
    char *t;

    t = (char *)tbl;
    if (tbl_size <= 0) {
	*ind = 0;
	return (0);
    }
    st = 0;
    end = tbl_size - 1;

    while (1) {
	int i, ret;

	i = (st + end) >> 1;
	if (st == i) {
	    int c;

	    c = cmp (t + st * item_size, ent);
	    *ind = st;
	    if (c == 0)
		return (1);
	    if (c > 0)
		return (0);
	    c = cmp (t + end * item_size, ent);
	    *ind = end;
	    if (c == 0) 
		return (1);
	    if (c > 0) 
		return (0);
	    *ind = end + 1;
	    return (0);
	}
	ret = cmp (ent, t + i * item_size);
	if (ret <= 0)
	    end = i;
	else
	    st = i;
    }
    return (-1);		/* never executed */
}

/************************************************************************

    Inserts an entry "ent" in the table "tblpt" that keeps the table in
    non-decreasing order (in terms of "cmp"). Returns the index of the new
    entry on success or -1 if malloc failed.

************************************************************************/

int MISC_table_insert (void *tblpt, void *ent, int (*cmp) (void *, void *)) {
    Misc_table *tbl;
    int n, es;
    char *t;
    int found, ind;

    if (MISC_table_new_entry (tblpt, NULL) == NULL)
	return (-1);

    tbl = (Misc_table *)tblpt;
    n = tbl->n_entries;
    t = (char *)tbl->table;
    es = tbl->entry_size;

    tbl->n_entries--;
    found = MISC_table_search (tblpt, ent, cmp, &ind);
    if (found) {		/* inserted after the last */
	ind++;
	while (ind < n - 1 && cmp (t + ind * es, ent) <= 0)
	    ind++;
    }
    tbl->n_entries = n;
    if (n - ind - 1 > 0)
	memmove (t + (ind + 1) * es, t + ind * es, (n - ind - 1) * es);
    memcpy (t + ind * es, ent, es);
    return (ind);
}

/************************************************************************

    Malloc function with error checking. The caller of this function does
    not need to perform exception handing. When the requested memory 
    resource is not available, this function either tries or terminates 
    the process depending on "Malloc_retry" set by MISC_malloc_retry. 
    This function prints the size to be allocated and the process stack
    before terminating the process. This function is event-safe.

************************************************************************/

void *MISC_malloc (size_t size) {
    char *p;
    char out_buf[512];

    while ((p = (char *)malloc (size)) == NULL) {
	if (Malloc_retry && size >= 0)
	    sleep (1);
	else
	    break;
    }
    if (p == NULL) {
	MISC_proc_printstack (getpid (), 512, out_buf);
	MISC_log ("MISC: Malloc %d bytes failed - stack:\n%s", size, out_buf);
	exit (1);
    }
    return ((void *)p);
}

/************************************************************************

    Event-safe free.

************************************************************************/

void MISC_free (void *p) {

    free (p);
}

/************************************************************************

    Controls whether MISC_malloc will retry in case of malloc failure.

************************************************************************/

void MISC_malloc_retry (int yes) {
    Malloc_retry = yes;
}

/************************************************************************

    Allocates a segment of memory of size "size" independently of "malloc".
    Returns the pointer to the allocated space on success or NULL on failure.

************************************************************************/

#include <sys/mman.h>
#include <errno.h>

void *MISC_ind_malloc (size_t size) {

#ifndef MAP_ANONYMOUS
    return (malloc (size));
#else
    char *p;
    p = (char *)mmap (NULL, size, PROT_READ | PROT_WRITE, 
					MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (p == NULL) {
	MISC_log ("MISC: mmap (MISC_ind_malloc) failed (size %d, errno %d)\n", 
						size, errno);
	return (NULL);
    }
    return ((void *)p);
#endif
}

/************************************************************************

    Frees a segment of memory allocated by MISC_ind_malloc. "p" must be
    returned from MISC_ind_malloc and "size" must be correct.
    Returns 0 on success or -1 on failure.

************************************************************************/

int MISC_ind_free (void *p, size_t size) {
    int ret;

#ifndef MAP_ANONYMOUS
    free (p);
    return (0);
#else
    ret = munmap (p, size);
    if (ret < 0)
	MISC_log ("MISC: munmap (MISC_ind_free) failed (p %p, size %d, errno %d)\n", 
						p, size, errno);
    return (ret);
#endif
}


