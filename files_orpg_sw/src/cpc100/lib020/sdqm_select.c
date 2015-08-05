
/******************************************************************

	file: sdqm_select.c

	This is part of the SDQM module - The simple data base 
	query management. This contains the select and other
	supporting functions.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/04/15 19:35:56 $
 * $Id: sdqm_select.c,v 1.21 2005/04/15 19:35:56 jing Exp $
 * $Revision: 1.21 $
 * $State: Exp $
 */  

#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <infr.h> 
#include <sdqm_def.h>

typedef struct {			/* struct for query message buffer */
    char *buf;				/* pointer to the buffer */
    int size;				/* current data size in the buffer */
    int buf_size;			/* current buffer size */
    int sec_cnt;			/* query section count */
    int error;				/* error code */
} Qm_buf_t;

static Qm_buf_t Qb = {NULL, 0, 0, 0, 0};	/* query message buffer */


static void Append_qm_buf (int len, char *data);
static void Free_qm_buf ();


/******************************************************************

    Description: This function appends "data" of length "size" to
		the query message buffer.

******************************************************************/

void SDQM_append_qm_buf (int size, char *data)
{
    Append_qm_buf (size, data);
    Qb.sec_cnt++;
}

/******************************************************************

    Description: This function starts a select session.

******************************************************************/

void SDQM_query_begin (char *db_name)
{
    SDQM_query q;
    SDQM_single_query sq;

    Free_qm_buf ();

#ifdef USE_PURIFY
    memset (&q, 0, sizeof (SDQM_query));
#endif

    q.msg_size = sizeof (SDQM_query);
    q.type = SDQM_QUERY;
    q.need_index = 0;
    q.n_queries = 0;
    Qb.sec_cnt = 0;
    strncpy (q.db_name, db_name, SDQM_DB_NAME_SIZE);
    q.db_name[SDQM_DB_NAME_SIZE - 1] = '\0';
    q.query_mode = 0;

    sq.size = sizeof (SDQM_single_query);
    sq.n_sections = 0;

    Append_qm_buf (sizeof (SDQM_query), (char *)&q);
    Append_qm_buf (sq.size, (char *)&sq);

    return;
}

/******************************************************************

    Description: This function sets the query mode bit fields.

    Input:	mode - the new query mode bit field.

******************************************************************/

void SDQM_set_query_mode (int mode)
{
    SDQM_query *q;

    q = (SDQM_query *)Qb.buf;
    if (q != NULL)
	q->query_mode = mode;
}

/******************************************************************

    Description: Selects an integer value.

    Input:	field - field enum.
		value - value to select.

******************************************************************/

void SDQM_select_int_value (int field, int value)
{

    SDQM_select_int_range (field, value, value);
    return;
}

/******************************************************************

    Description: Selects an integer range.

    Input:	field - field enum.
		min - min value.
		max - max value.

******************************************************************/

void SDQM_select_int_range (int field, int min, int max)
{
    SDQM_query_int qi;

    qi.size = sizeof (SDQM_query_int);
    qi.field = field;
    qi.type = SDQM_QFT_INT;
    qi.unused = 0;
    qi.min = min;
    qi.max = max;
    SDQM_append_qm_buf (qi.size, (char *)&qi);
    return;
}

/******************************************************************

    Description: Selects a short integer value.

    Input:	field - field enum.
		value - value to select.

******************************************************************/

void SDQM_select_short_value (int field, short value)
{

    SDQM_select_short_range (field, value, value);
    return;
}

/******************************************************************

    Description: Selects a short integer range.

    Input:	field - field enum.
		min - min value.
		max - max value.

******************************************************************/

void SDQM_select_short_range (int field, short min, short max)
{
    SDQM_query_short qs;

    qs.size = sizeof (SDQM_query_short);
    qs.field = field;
    qs.type = SDQM_QFT_SHORT;
    qs.unused = 0;
    qs.min = min;
    qs.max = max;
    SDQM_append_qm_buf (qs.size, (char *)&qs);
    return;
}

/******************************************************************

    Description: Selects a float value.

    Input:	field - field enum.
		value - value to select.

******************************************************************/

void SDQM_select_float_value (int field, float value)
{

    SDQM_select_float_range (field, value, value);
    return;
}

/******************************************************************

    Description: Selects a float range.

    Input:	field - field enum.
		min - min value.
		max - max value.

******************************************************************/

void SDQM_select_float_range (int field, float min, float max)
{
    SDQM_query_float qs;

    qs.size = sizeof (SDQM_query_float);
    qs.field = field;
    qs.type = SDQM_QFT_FLOAT;
    qs.unused = 0;
    qs.min = min;
    qs.max = max;
    SDQM_append_qm_buf (qs.size, (char *)&qs);
    return;
}

/******************************************************************

    Description: Selects a string value.

    Input:	field - field enum.
		str - value to select.

******************************************************************/

void SDQM_select_string_value (int field, char *str)
{

    SDQM_select_string_range (field, str, str);
    return;
}

/******************************************************************

    Description: Selects a string range.

    Input:	field - field enum.
		str1 - min value.
		str2 - max value.

******************************************************************/

void SDQM_select_string_range (int field, char *str1, char *str2)
{
    SDQM_query_string qs;
    int len1, len2, size, npads;

    len1 = strlen (str1);
    if (str1 == str2 ||
	strcmp (str1, str2) == 0)
	len2 = -1;			/* identical string */
    else
	len2 = strlen (str2);

    size = sizeof (SDQM_query_string);
    qs.field = field;
    qs.type = SDQM_QFT_STRING;
    qs.unused = 0;
    qs.str1_off = size;
    size += len1 + 1;
    qs.str2_off = size;
    if (len2 >= 0)
	size += len2 + 1;
    else
	qs.str2_off = qs.str1_off;
    qs.size = ALIGNED_SIZE (size);
    npads = qs.size - size;
    Append_qm_buf (sizeof (SDQM_query_string), (char *)&qs);
    Append_qm_buf (len1 + 1, str1);
    if (len2 >= 0)
	Append_qm_buf (len2 + 1, str2);
    Append_qm_buf (npads, NULL);
    Qb.sec_cnt++;
    return;
}

/******************************************************************

    Description: Terminates a selection session by completing
		the message headers.

******************************************************************/

void SDQM_query_end ()
{
    SDQM_query *q;
    SDQM_single_query *sq;
    int prev_size;

    q = (SDQM_query *)Qb.buf;
    prev_size = q->msg_size;
    q->msg_size = Qb.size;
    q->n_queries++;
    sq = (SDQM_single_query *)(Qb.buf + prev_size);
    sq->size = q->msg_size - prev_size;
    sq->n_sections = Qb.sec_cnt;

    return;
}

/******************************************************************

    Description: Starts the next query definition.

******************************************************************/

void SDQM_query_next ()
{
    SDQM_single_query sq;

    SDQM_query_end ();
    Qb.sec_cnt = 0;

    sq.size = sizeof (SDQM_single_query);
    sq.n_sections = 0;

    Append_qm_buf (sq.size, (char *)&sq);

    return;
}

/******************************************************************

    Description: Executes a query locally with SDQM_INDEX_RESULT_ONLY
		results.

    Input:	max_list_size - maximum list size in the results.

    Output:	results - output results message.

    Return:	length of the result message on query success or 
		a negative SDQM error code.

******************************************************************/

int SDQM_execute_query_index (int max_list_size, SDQM_index_results **results)
{
    SDQM_query *q;

    if (Qb.error != 0)
	return (Qb.error);

    SDQM_query_end ();
    q = (SDQM_query *)Qb.buf;
    q->max_list_size = max_list_size;

    return (SDQM_process_query (Qb.buf, Qb.size, 
			SDQM_INDEX_RESULT_ONLY, (void **)results));
}

/******************************************************************

    Description: Executes a query locally with SDQM_SERIALIZED_RESULT
		results.

    Input:	max_list_size - maximum list size in the results.
		need_index - boolean, needs to return index list.

    Output:	results - output results message.

    Return:	length of the result message or SDQM_MALLOC_FAILED
		if malloc failed.

******************************************************************/

int SDQM_execute_query_local (int max_list_size, 
			int need_index, SDQM_query_results **results)
{
    SDQM_query *q;

    if (Qb.error != 0)
	return (Qb.error);

    SDQM_query_end ();
    q = (SDQM_query *)Qb.buf;
    q->max_list_size = max_list_size;
    q->need_index = need_index;

    return (SDQM_process_query (Qb.buf, Qb.size, 
			SDQM_SERIALIZED_RESULT, (void **)results));
}

/******************************************************************

    Description: Appends "len" bytes of data in "data" into the 
		query message buffer.

    Input:	len - length of the data to append.
		data - pointer to the data.

******************************************************************/

#define QB_SIZE_EXTRA  512	/* extra space allocated to avoid 
				   frequent mallocs */

static void Append_qm_buf (int len, char *data)
{

    if (len <= 0)
	return;
    if (Qb.size + len > Qb.buf_size) {
	int new_size;
	char *pt;

	new_size = Qb.size + len + QB_SIZE_EXTRA;
	pt = malloc (new_size);
	if (pt == NULL) {
	    Qb.error = SDQM_MALLOC_FAILED;
	    return;
	}
	if (Qb.size > 0)
	    memcpy (pt, Qb.buf, Qb.size);
	if (Qb.buf != NULL) {
	    free (Qb.buf);
	    Qb.buf = NULL;
	}
	Qb.buf_size = new_size;
	Qb.buf = pt;
    }
    if (data != NULL)
	memcpy (Qb.buf + Qb.size, data, len);
    Qb.size += len;

    return;
}

/******************************************************************

    Description: Frees the query message buffer.

******************************************************************/

static void Free_qm_buf ()
{

    if (Qb.buf != NULL) {
	free (Qb.buf);
	Qb.buf = NULL;
    }
    Qb.buf_size = 0;
    Qb.size = 0;
    Qb.sec_cnt = 0;
    Qb.error = 0;
    return;
}



