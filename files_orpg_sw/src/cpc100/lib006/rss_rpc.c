
/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/10 15:08:23 $
 * $Id: rss_rpc.c,v 1.31 2014/11/10 15:08:23 steves Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 */

/************************************************************************

    A Generic RPC function. Function return value is treated as another
    output argument.

	s - (i, o, r); Treated as a null terminated string pointer which 
	    is dereferenced and the string is passed over.
	ba - (i, o, r); Size required; Treated as a byte array which 
	    is dereferenced and data of "size" bytes are passed over.
	p - (i, r); Generic pointer value passed over. Byte swap NOT
	    performed because it is useless.
	ia - (i, o, r); Size optional with default 1; Treated as an
	    int array which is dereferenced and data of "size" 4-byte
	    ints are passed over; Byte swap performed.
	i - (i, r); 4-byte integer value passed over. Byte swap 
	    performed.
	v - (i, o, r); Byte array of variable size; Such argument must
	    be a pointer to RSS_variable_size_arg. Array size is 
	    explicitly specified in the argument. The data is treated 
	    as a byte array.
	user_type - (i, o, r); User defined type (C struct); Variable
	    sized structures are suported. Serialization and byteswap
	    are performed automatically.

	In order to perform byte swap, I let the data string carry the
	endianness info. Only if the data come from a host of different
	byte order, byte swap is done (Instead of always using big
	endian format). This simplifies my code and improves efficiency.
	Further host info (e.g. hardware code) can be carried for more 
	advanced data format changes (e.g structures, macros) in the 
	future.

************************************************************************/

#include <config.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h> 
#include <dlfcn.h>

#include "rmt_user_def.h"
#include <rmt.h>
#include <rss.h>
#include "rss_def.h"
#include <misc.h>
#include <smi.h>


#define ARG_IN 1
#define ARG_OUT 2
#define ARG_RET 4

enum {ARG_I = 1, ARG_IA, ARG_S, ARG_BA, ARG_P, ARG_V, ARG_VOID};

#define TYPE_SIZE 32
#define MAX_N_ARGS 16

typedef struct {			/* load function struct */
    char *name;				/* function/lib name */
    void *func;				/* function pointer */
    void *handle;			/* lib file handle */
} Func_struct;

static void *Func_tblid = NULL;		/* function table id */
static Func_struct *Funcs;		/* function/lib table */
static int N_funcs;			/* size of Funcs */

static int Need_byteswap;		/* caller needs to byte swap data */
static char *Smi_func_name = NULL;	/* SMI function name for byteswap */

static int Call_level = 0;		/* recur call level on this machine */

typedef struct {			/* buffer management struct */
    char *st_buf;			/* static buffer */
    char *buf;				/* buffer currently used */
    int len;				/* data length */
    int size;				/* buffer size */
} Buffer_t;

typedef struct {			/* return type attributes */
    char *buf;				/* position (in the shared in/out 
					   buffer) where return data to put */
    int type;				/* type ARG_* */
    int size;				/* size of the return data */
    char *type_name;			/* name of user defined type */
} Ret_attr_t;

typedef struct {			/* variable size argument attributes */
    int type;				/* type ARG_* */
    int off;				/* offset in shared in/out buffer */
    char *type_name;			/* name of user defined type */
} Variable_size_attr_t;

#define VOID_OFFSET -1

#define DEL_ALL ((void *)0)		/* for calling Save_free_pointers */
#define DEL_HIGH_LEVEL ((void *)1)

#ifdef LITTLE_ENDIAN_MACHINE
static unsigned int Local_endian = 0;
#else
static unsigned int Local_endian = 0x80000000;
#endif

static void (*Get_stdout) (char *std) = NULL;

static int Form_arg_string (char *arg_f, va_list args, Buffer_t *b);
static int Parse_arg_format (char *cpt, 
			int *type, int *io, int *size, char *type_name);
static int Add_to_buffer (Buffer_t *b, void *data, int len);
static void Free_buffer (Buffer_t *b);
static int rss_rpc (int arglen, char *arg, char **ret_str);

static int Prepare_args (char *arg_f, char *p, char *args[], 
	Buffer_t *b, Ret_attr_t *r_attr, Variable_size_attr_t *vsa);
static int Get_function_pt (char *name, void **f_p);
static int Set_output_data (char *arg_f, 
				va_list vargs, int len, char *ret_str);
static int Add_to_table (char *fname, void *f_p, void *handle);
static int Cmp_name (void *e1, void *e2);
static void Int_byte_swap (int *p, int cnt);
static int Function_call_interface (int n_args, int *args, void *f_p);
static int Rss_rpc_internal (va_list vargs, char *arg_f, 
					char *func_name, int local);
static int Dereference_func_output (int type, char *type_name, 
					char *p, Buffer_t *crb);
static char *Get_type_name_pointer (char *p, char *type_name);
static int Serialize_data (char *type_name, char *in, char **out, int *fr);
static int Deserialize_data (char *type_name, char *in, 
				int size, char **out, int need_byteswap);
static int Load_smi_function ();
static int Save_free_pointers (char *type_name, char *save_p);
static int Get_std_msg (char *std, int buf_size);
static int Free_and_return (Buffer_t *b, int ret_value);


/********************************************************************

    Sets stdout/stderr output function.

********************************************************************/

void RSS_rpc_stdout_redirect (void (*get_stdout) (char *)) {
    Get_stdout = get_stdout;
}

/********************************************************************

    Returns the "Need_byteswap" flag.

********************************************************************/

int RSS_rpc_need_byteswap () {
    return (Need_byteswap);
}

/********************************************************************

    Sets SMI function name.

********************************************************************/

int RSS_set_SMI_func_name (char *name) {

    if (Smi_func_name != NULL && strlen (Smi_func_name) > 1 && 
				strcmp (Smi_func_name + 1, name) == 0)
	return (0);
    if (Smi_func_name != NULL)
	free (Smi_func_name);
    Smi_func_name = (char *)malloc (strlen (name) + 2);
    if (Smi_func_name == NULL)
	return (RSS_MALLOC_FAILED);
    *Smi_func_name = '@';
    strcpy (Smi_func_name + 1, name);
    return (0);
}

/********************************************************************

    Generic RPC function. The name format: host:lib_name,file_name.
    Where host and lib_name are optional.

********************************************************************/

int RSS_rpc (char *name, char *arg_f, ...) {
    va_list vargs;
    int ret, local;
    char host_name[TMP_SIZE];	/* host name */
    char func_name[TMP_SIZE];	/* function name */

#ifdef TEST_OPTIONS
    if (MISC_test_options ("PROFILE")) {
	char b[128];
	MISC_string_date_time (b, 128, (const time_t *)NULL);
	fprintf (stderr, "%s PROF: RSS_rpc %s\n", b, name);
    }
#endif

    /* parse "name" to find the host name and the function name */
    ret = RSS_find_host_name (name, host_name, func_name, TMP_SIZE);
    if (ret == RSS_FAILURE)	/* can not find host name */
	return (RSS_HOSTNAME_FAILED);
    if (strlen (func_name) == 0)	/* void function name */
	return (RSS_BAD_PATH_NAME);
    if (ret == LOCAL_HOST_IMPLICIT || ret == LOCAL_HOST_EXPLICIT) {
	char *cpt = name;
	while (*cpt != '\0') {	/* func_name (with $HOME/) cannot be used */
	    if (*cpt == ':') {
		strncpy (func_name, cpt + 1, TMP_SIZE);
		func_name[TMP_SIZE - 1] = '\0';
		break;
	    }
	    cpt++;
	}
    }
    if (ret == LOCAL_HOST_IMPLICIT || ret == LOCAL_HOST_EXPLICIT)
	local = 1;	/* The same address space loading */
    else {
	/* open a connection to the rmt server */
	if ((ret = RMT_create_connection (host_name)) < 0)
	    return (ret);
	local = 0;
    }

    va_start (vargs, arg_f);
    ret = Rss_rpc_internal (vargs, arg_f, func_name, local);
    va_end (vargs);
    return (ret);
}

/********************************************************************

    Generic LPC function. The name format: lib_name,file_name.
    Where lib_name is optional.

********************************************************************/

int RSS_lpc (char *name, char *arg_f, ...) {
    va_list vargs;
    int ret;
    char host_name[TMP_SIZE];	/* host name */

    if (gethostname (host_name, TMP_SIZE) < 0) {
	MISC_log ("gethostname failed (errno %d)", errno);
	return (RSS_RPC_GETHOSTNAME_FAILED);
    }
    /* open a connection to the rmt server */
    RMT_sharing_client (0);
    ret = RMT_create_connection (host_name);
    RMT_sharing_client (-1);
    if (ret < 0)
	return (ret);

    va_start (vargs, arg_f);
    ret = Rss_rpc_internal (vargs, arg_f, name, 0);
    va_end (vargs);
    return (ret);
}

/********************************************************************

    Generic dynamicly loaded function call interface. The name 
    format: lib_name,file_name. Where lib_name is optional.

********************************************************************/

int RSS_pc (char *name, char *arg_f, ...) {
    va_list vargs;
    int ret;

    va_start (vargs, arg_f);
    ret = Rss_rpc_internal (vargs, arg_f, name, 1);
    va_end (vargs);
    return (ret);
}

/********************************************************************

    Generic RPC function. Internal implementation.

********************************************************************/

#define MAX_ARG_F_SIZE 200
#define INIT_BUF_SIZE 512

static int Rss_rpc_internal (va_list vargs, char *arg_f, 
					char *func_name, int local) {
    Buffer_t b;
    char *ret_str;
    ALIGNED_t buf[ALIGNED_T_SIZE (INIT_BUF_SIZE)];
    int len;
    char *a_f, af_buf[MAX_ARG_F_SIZE];

    Call_level++;
    if (Call_level == 1)
	Save_free_pointers (DEL_ALL, NULL);	
			/* free pointers used for previous call */

    /* expand the sizes in the argument format string */
    a_f = arg_f;
    if (strstr (arg_f, "%d") != NULL) {
	int cnt, s;
	char *sp, *dp, *p;

	sp = arg_f;
	dp = af_buf;
	cnt = strlen (arg_f);
	while ((p = strstr (sp, "%d")) != NULL) {
	    if (dp - af_buf + p - sp + 16 > MAX_ARG_F_SIZE) {
		Call_level--;
		return (RSS_RPC_FORMAT_ERROR);
	    }
	    memcpy (dp, sp, p - sp);
	    dp += p - sp;
	    s = va_arg (vargs, int);
	    sprintf (dp, "%d", s);
	    dp += strlen (dp);
	    sp += p - sp + 2;
	    cnt -= p - sp + 2;
	}
	if (dp - af_buf + cnt >= MAX_ARG_F_SIZE) {
	    Call_level--;
	    return (RSS_RPC_FORMAT_ERROR);
	}
	strcpy (dp, sp);
	a_f = af_buf;
    }

    b.st_buf = b.buf = (char *)buf;
    b.len = 0;
    b.size = INIT_BUF_SIZE;
    if (Smi_func_name != NULL &&
	Add_to_buffer (&b, Smi_func_name, strlen (Smi_func_name) + 1) < 0) {
	Call_level--;
	return (RSS_MALLOC_FAILED);
    }
    if (Add_to_buffer (&b, func_name, strlen (func_name) + 1) < 0 ||
	Add_to_buffer (&b, a_f, strlen (a_f) + 1) < 0) {
	Call_level--;
	return (RSS_MALLOC_FAILED);
    }
    len = Form_arg_string (a_f, vargs, &b);
    va_end (vargs);
    if (len < 0) {
	Free_buffer (&b);
	Call_level--;
	return (len);
    }

    if (local)
	len = rss_rpc (b.len, b.buf, &ret_str);
    else
	len = Rmt_user_func_22 (b.len, b.buf, &ret_str);
    Free_buffer (&b);

    if (len < 0) {
	Call_level--;
	return (len);
    }
    Set_output_data (a_f, vargs, len, ret_str);
    Save_free_pointers (DEL_HIGH_LEVEL, NULL);	

    Call_level--;
    return (0);
}

/********************************************************************

    The remote function.

********************************************************************/

int Rmt_user_func_22_server (int arglen, char *arg, char **ret_str) {
    return (rss_rpc (arglen, arg, ret_str));
}

/********************************************************************

    The remote function. This function can be recursively called for
    supporting RSS_rpc calls with in a RSS_rpc function.

********************************************************************/

#define MAX_RECURSIVE_CALL_LEVEL 16
#define PERM_BUF_SIZE 512

static int rss_rpc (int arglen, char *arg, char **ret_str) {
    Buffer_t crb;
    static char *buf = NULL;
    int n_args, ret, type, i;
    char *name, *arg_f, *p, *args[MAX_N_ARGS];
    Ret_attr_t r_attr;		/* attributes of function return */
    void *f_p;
    Variable_size_attr_t vsa[MAX_N_ARGS];	/* variable size argument */
    unsigned int *cntl_p;

    if (buf == NULL) {		/* a permanent buffer for efficiency */
	buf = (char *)malloc (PERM_BUF_SIZE);
	if (buf == NULL)
	    return (RSS_MALLOC_FAILED);
    }

    if (Call_level >= MAX_RECURSIVE_CALL_LEVEL)	
	return (RSS_RPC_RECURSIVE_CALLS);
    Call_level++;

    if (Call_level == 1)
	Save_free_pointers (DEL_ALL, NULL);	

    p = arg;
    if (*p == '@') {		/* get SMI function name */
	if (Smi_func_name == NULL || strcmp (Smi_func_name, p) != 0) {
	    if (Smi_func_name != NULL)
		free (Smi_func_name);
	    Smi_func_name = malloc (strlen (p) + 1);
	    if (Smi_func_name == NULL) {
		Call_level--;
		return (RSS_MALLOC_FAILED);
	    }
	    strcpy (Smi_func_name, p);
	}
	p += ALIGNED_SIZE (strlen (p) + 1);
    }

    name = p;			/* RPC function name */
    arg_f = name + ALIGNED_SIZE (strlen (name) + 1);
    p = arg_f + ALIGNED_SIZE (strlen (arg_f) + 1);

    if (Call_level == 1) {
	crb.st_buf = crb.buf = (char *)buf;
	crb.size = PERM_BUF_SIZE;
    }
    else {
	crb.st_buf = crb.buf = NULL;
	crb.size = 0;		/* malloc */
    }
    crb.len = 0;
    memset (&r_attr, 0, sizeof (Ret_attr_t));
    n_args = Prepare_args (arg_f, p, args, &crb, &r_attr, vsa);
    if (n_args < 0)
	return (Free_and_return (&crb, n_args));

    /* find function pointer */
    ret = Get_function_pt (name, &f_p);
    if (ret < 0)
	return (Free_and_return (&crb, ret));

    if (Call_level == 1)
	Get_std_msg (NULL, 0);	/* flush stdout and stderr */

    ret = Function_call_interface (n_args, (int *)args, f_p);
    if (r_attr.type != ARG_VOID)
	*((int *)r_attr.buf) = ret;

    type = r_attr.type;		/* dereference function return */
    if (type == ARG_S || type == ARG_V || type == ARG_BA || type == ARG_IA) {
	int off, s_off;

	off = r_attr.buf - crb.buf;	/* return field offset */
	s_off = crb.len;		/* offset of the deserialized in crb */
	if (type == ARG_S || type == ARG_V) {
	    r_attr.size = Dereference_func_output (type, r_attr.type_name, 
							(char *)ret, &crb);
	    if (r_attr.size < 0)
		return (Free_and_return (&crb, r_attr.size));
	    if (r_attr.size == 0)
		s_off = VOID_OFFSET;
	}
	else {
	    if (Add_to_buffer (&crb, (char *)ret, r_attr.size) < 0)
		return (Free_and_return (&crb, RSS_MALLOC_FAILED));
	}
	*((int *)(crb.buf + off)) = s_off;
    }

    for (i = 0; i < MAX_N_ARGS; i++) {	/* dereference pointer output */
	int off, s_off;
	char *p;
	off = vsa[i].off;		/* pointer field offset */
	if (off < 0)
	    break;
	s_off = crb.len;		/* offset of the dereferenced in crb */
	memcpy (&p, crb.buf + off, sizeof (int));

	if ((ret = Dereference_func_output (vsa[i].type, vsa[i].type_name, 
							p, &crb)) < 0)
	    return (Free_and_return (&crb, ret));
	if (ret == 0)
	   s_off  = VOID_OFFSET;
	*((int *)(crb.buf + off)) = s_off;
    }

    cntl_p = (unsigned int *)(crb.buf);
    if (Call_level == 1) {		/* returns stdout and stderr */
	char buf[4096];
	int len = Get_std_msg (buf, 4096);
	if (len > 0) {
	    if (Add_to_buffer (&crb, buf, len) < 0)
		return (Free_and_return (&crb, RSS_MALLOC_FAILED));
	    cntl_p = (unsigned int *)(crb.buf);
	    *cntl_p = *cntl_p | ALIGNED_SIZE (len);
	}
    }
    if (!Local_endian)
	Int_byte_swap ((int *)cntl_p, 1);

    if (crb.st_buf != crb.buf && crb.buf != NULL &&
	Save_free_pointers ("", crb.buf) < 0)
	return (Free_and_return (&crb, RSS_MALLOC_FAILED));

    Save_free_pointers (DEL_HIGH_LEVEL, NULL);	
    *ret_str = crb.buf;
    Call_level--;
    return (crb.len);
}

/**************************************************************************

    Frees "b", decrements Call_level and returns "ret_value".

**************************************************************************/

static int Free_and_return (Buffer_t *b, int ret_value) {

    Free_buffer (b);
    Call_level--;
    return (ret_value);
}

/****************************************************************************

    Dereferences data pointed by "p" of ("type", "type_name") and appends
    to "crb". Type must be ARG_S or ARG_V. The pointer is freed if
    needed. Returns the size of the serialized data on success or a
    negative error code.

****************************************************************************/

static int Dereference_func_output (int type, char *type_name, 
					char *p, Buffer_t *crb) {
    int size, fr;
    RSS_variable_size_arg *va, tva;
    char *pout;

    va = NULL;
    pout = p;
    size = 0;
    fr = -1;			/* not necessary - shut up gcc */
    if (p == NULL)
	size = 0;
    else {
	if (type == ARG_S)
	    size = strlen (p) + 1;
	if (type == ARG_V) {
	    if (type_name == NULL)
		va = (RSS_variable_size_arg *)p;
	    else {
		tva.size = Serialize_data (type_name, p, &(tva.data), &fr);
		if (tva.size < 0)
		    return (tva.size);
		tva.free_this = tva.free_data = 0;
		va = &tva;
		pout = (char *)va;
	    }
	    size = sizeof (RSS_variable_size_arg);
	}
    }
    if (Add_to_buffer (crb, pout, size) < 0 ||
	(va != NULL &&
	 Add_to_buffer (crb, va->data, va->size) < 0)) {
	return (RSS_MALLOC_FAILED);
    }
    if (fr == 1)
	free (va->data);
    if (va != NULL && type_name == NULL) {
	if (va->free_data && va->data != NULL)
	    free (va->data);
	if (va->free_this)
	    free (va);
    }
    if (va != NULL)
	size += va->size;
    return (size);
}

/********************************************************************

    Gets the function pointer of "name" and returns it in "f_p". Returns
    0 on success or a negative error number. We use a single table for
    library files and function names assuming they are always different.
    Otherwise code enhancement is needed.

********************************************************************/

static int Get_function_pt (char *name, void **f_p) {
    char file[128], *fname;
    void *handle;
    char *error;
    Func_struct item;
    int ind;

    if (Func_tblid == NULL &&
	(Func_tblid = MISC_open_table (sizeof (Func_struct), 
			32, 1, &N_funcs, (char **)&Funcs)) == NULL)
	return (RSS_MALLOC_FAILED);

    fname = name;
    while (*fname != '\0' && *fname != ',')
	fname++;
    if (*fname == '\0') {
	file[0] = '@';			/* default lib file name */
	file[1] = '\0';
	fname = name;
    }
    else {
	if (fname - name >= 128 || fname[1] == '\0')
	    return (RSS_RPC_NAME_ERROR);
	memcpy (file, name, fname - name);
	file[fname - name] = '\0';
	fname++;
	if (*fname == '\0')
	    return (RSS_RPC_NAME_ERROR);
    }

    item.name = fname;		/* search for function name */
    if (MISC_table_search (Func_tblid, &item, Cmp_name, &ind)) {
	*f_p = Funcs[ind].func;
	return (0);
    }

    item.name = file;		/* search for open file */
    handle = NULL;
    if (MISC_table_search (Func_tblid, &item, Cmp_name, &ind))
	handle = Funcs[ind].handle;

    if (handle == NULL) {
	char *f;
	if (file[0] == '@')
	    f = NULL;
	else
	    f = file;
	if ((handle = dlopen (f, RTLD_LAZY)) == NULL) {
	    MISC_log ("%s\n", dlerror ());
	    return (RSS_RPC_DLOPEN);
	}
	if (Add_to_table (file, NULL, handle) < 0) {
	    dlclose (handle);
	    return (RSS_MALLOC_FAILED);
	}
    }

    *f_p = dlsym (handle, fname);
    if ((error = dlerror()) != NULL) {
	MISC_log ("%s\n", error);
	return (RSS_RPC_DLSYM);
    }
    if (Add_to_table (fname, *f_p, NULL) < 0)
	return (RSS_MALLOC_FAILED);

    return (0);
}

/********************************************************************

    Adds a new item to the function table.

********************************************************************/

static int Add_to_table (char *fname, void *f_p, void *handle) {
    Func_struct item;

    item.name = (char *)malloc (strlen (fname) + 1);
    if (item.name == NULL)
	return (-1);
    strcpy (item.name, fname);
    item.func = f_p;
    item.handle = handle;
    if (MISC_table_insert (Func_tblid, (void *)&item, Cmp_name) < 0) {
	free (item.name);
	return (-1);
    }
    return (0);
}

/************************************************************************

    Name comparison function.

************************************************************************/

static int Cmp_name (void *e1, void *e2) {
    Func_struct *f1, *f2;
    f1 = (Func_struct *)e1;
    f2 = (Func_struct *)e2;
    return (strcmp (f1->name, f2->name));
}

/********************************************************************

    Prepares the arguments for calling the function. The necessary
    buffers for the output are allocated in "b". "arg_f" is the format
    string, "p" points to the passed-in argument string. The args are
    returned in "args". The function returns the number of args on
    success or a negative error number. "r_attr" returns the attribute
    of the return value. "vsa" returns the offsets and type of all
    pointer type arguments excluding return value. The arrays is
    terminated with a negative "off" field.

    New buffers are allocated for all args except input only. If an
    arg is of "io", its content is copied from the input buffer to the
    output buffer. The function return is treated as an output arg.
    Because b->buf may be reallocated, we first assume NULL location
    (args[arg_cnt] = (char *0) + ...) and finally set the real
    pointers (args[i] = b->buf + (unsigned int)args[i]).

********************************************************************/

static int Prepare_args (char *arg_f, char *p, char *args[], 
	Buffer_t *b, Ret_attr_t *r_attr, Variable_size_attr_t *vsa) {
    char type_name[TYPE_SIZE], *cpt, *cp_p[MAX_N_ARGS];
    int len, size, io, type, cp_len[MAX_N_ARGS], i;
    int str_cnt, arg_cnt, cnt;
    unsigned int control;

    control = *((int *)p);		/* get control and data endianness */
    if (!Local_endian)
	Int_byte_swap ((int *)&control, 1);
    p += ALIGNED_SIZE (sizeof (int));
    if (Add_to_buffer (b, &Local_endian, 4) < 0)
	return (RSS_MALLOC_FAILED);	/* the first part is endianness */
    if ((control & 0x80000000) != Local_endian)
	Need_byteswap = 1;
    else
	Need_byteswap = 0;

    r_attr->type = ARG_VOID;
    vsa[0].off = -1;
    str_cnt = arg_cnt = cnt = 0;
    cpt = arg_f;
    while ((len = Parse_arg_format (cpt, &type, &io, &size, type_name)) > 0) {
	cpt += len;
	cp_len[arg_cnt] = -1;		/* no copy is needed */
	if (io & ARG_IN) {		/* input argument */
	    if (io & (~ARG_IN))
		cp_p[arg_cnt] = p;
	    switch (type) {
		RSS_variable_size_arg *va;

		case ARG_I:
		case ARG_P:
		if (type == ARG_I && Need_byteswap)
		    Int_byte_swap ((int *)p, 1);
		args[arg_cnt] = (char *)(*((int *)p));
		p += ALIGNED_SIZE (sizeof (int));
		cp_len[arg_cnt] = sizeof (int);
		break;

		case ARG_S:
		case ARG_V:
		if (((control >> cnt) & 0x1))
		    args[arg_cnt] = NULL;
		else {
		    if (type == ARG_S) {
			args[arg_cnt] = p;
			cp_len[arg_cnt] = strlen (p) + 1;
		    }
		    else {
			va = (RSS_variable_size_arg *)p;
			if (Need_byteswap)
			    Int_byte_swap (&(va->size), 1);

			if (type_name[0] == '\0') {
			    va->data = p + sizeof (RSS_variable_size_arg);
			    args[arg_cnt] = p;
			}
			else {
			    char *out;
			    int ret = Deserialize_data (type_name, 
					p + sizeof (RSS_variable_size_arg), 
					va->size, 
					&out, Need_byteswap);
			    if (ret < 0)
				return (ret);
			    args[arg_cnt] = out;
			}
			cp_len[arg_cnt] = sizeof (RSS_variable_size_arg) + 
								va->size;
		    }
		    p += ALIGNED_SIZE (cp_len[arg_cnt]);
		}
		break;

		case ARG_BA:
		case ARG_IA:
		if (((control >> cnt) & 0x1))
		    args[arg_cnt] = NULL;
		else {
		    if (type == ARG_IA && Need_byteswap)
			Int_byte_swap ((int *)p, size / sizeof (int));
		    args[arg_cnt] = p;
		    p += ALIGNED_SIZE (size);
		    cp_len[arg_cnt] = size;
		}
		break;
	    }
	}
	if (io & (~ARG_IN)) {		/* not input only arg */
	    switch (type) {

		case ARG_I:
		case ARG_P:
		case ARG_S:
		case ARG_V:
		if (Add_to_buffer (b, NULL, sizeof (int)) < 0)
		    return (RSS_MALLOC_FAILED);
		args[arg_cnt] = (char *)0 + b->len - sizeof (int);
		if ((type == ARG_S || type == ARG_V) && (io & ARG_OUT)) {
		    vsa[str_cnt].type_name = 
				Get_type_name_pointer (cpt - len, type_name);
		    vsa[str_cnt].type = type;
		    vsa[str_cnt++].off = (int)args[arg_cnt];
		    vsa[str_cnt].off = -1;
		}
		break;

		case ARG_BA:
		case ARG_IA:
		if (Add_to_buffer (b, NULL, size) < 0)
		    return (RSS_MALLOC_FAILED);
		args[arg_cnt] = (char *)0 + b->len - size;
		break;
	    }
	    if (io & ARG_RET) {
		r_attr->type = type;
		r_attr->type_name = 
			Get_type_name_pointer (cpt - len, type_name);
		r_attr->size = size;
		r_attr->buf = args[arg_cnt];
		arg_cnt--;		/* not put in "args" */
	    }
	}
	else
	    cp_len[arg_cnt] = -2;	/* not in the output buffer */
	arg_cnt++;
	cnt++;
    }
    if (len < 0)
	return (len);
    for (i = 0; i < arg_cnt; i++) {
	if (cp_len[i] != -2) {
	    args[i] = b->buf + (unsigned int)args[i];
	    if (cp_len[i] > 0)
		memcpy (args[i], cp_p[i], cp_len[i]);
	}
    }
    r_attr->buf = b->buf + (unsigned int)r_attr->buf;
    return (arg_cnt);
}

/***************************************************************************

    Returns the pointer to string "type_name" in string "p". The string in
    "p" is null terminated. If the "type_name" is empty or the string is
    not found in "p", NULL is returned.

***************************************************************************/

static char *Get_type_name_pointer (char *p, char *type_name) {
    char *tmp;

    tmp = NULL;
    if (type_name[0] != '\0' &&
	(tmp = strstr (p, type_name)) != NULL) {
	tmp[strlen (type_name)] = '\0';
    }
    return (tmp);
}

/********************************************************************

    Creates the calling argument string. Returns 0 on success or a
    negative error number.

********************************************************************/

static int Form_arg_string (char *arg_f, va_list vargs, Buffer_t *b) {
    char type_name[TYPE_SIZE], *cpt;
    unsigned int control, cntl_off;
    int len, type, size, io, cnt, rcnt;

    cntl_off = b->len;
    control = Local_endian;
    if (Add_to_buffer (b, &control, sizeof (int)) < 0)
					/* first part - control */
	return (RSS_MALLOC_FAILED);

    cnt = rcnt = 0;
    cpt = arg_f;
    while ((len = Parse_arg_format (cpt, &type, &io, &size, type_name)) > 0) {
	cpt += len;
	if (io & ARG_RET) {	/* further verification need to be added */
	    if (cnt > 0 || 	/* must be before the first arg */
		rcnt > 0)	/* only one spec is allowed */
		return (RSS_RPC_FORMAT_ERROR);
	    rcnt++;
	}
	if (cnt > MAX_N_ARGS)
	    return (RSS_RPC_FORMAT_ERROR);
	switch (type) {
	    int i;
	    char *p;

	    case ARG_I:
	    case ARG_P:
	    i = va_arg (vargs, int);
	    if (io & ARG_OUT)
		return (RSS_RPC_FORMAT_ERROR);
	    if ((io & ARG_IN) &&
		Add_to_buffer (b, &i, sizeof (int)) < 0)
		return (RSS_MALLOC_FAILED);
	    break;

	    case ARG_S:
	    case ARG_V:
	    p = va_arg (vargs, char *);
	    if (io & ARG_IN) {
		if (io & ARG_OUT)
		    return (RSS_RPC_FORMAT_ERROR);
		if (p == NULL)
		    control |= (1 << cnt);
		else if (type == ARG_S) {
		    if (Add_to_buffer (b, p, strlen (p) + 1) < 0)
			return (RSS_MALLOC_FAILED);
		}
		else {		/* type == ARG_V */
		    RSS_variable_size_arg *va, tva;
		    int fr = -1;

		    if (type_name[0] == '\0')
			va = (RSS_variable_size_arg *)p;
		    else {
			tva.size = Serialize_data (type_name, p, &(tva.data), &fr);
			if (tva.size < 0)
			    return (tva.size);
			tva.free_this = tva.free_data = 0;
			va = &tva;
		    }
		    if (Add_to_buffer (b, (char *)va, 
				sizeof (RSS_variable_size_arg)) < 0 ||
			Add_to_buffer (b, va->data, va->size) < 0)
			return (RSS_MALLOC_FAILED);
		    if (fr == 1)
			free (va->data);
		}
	    }
	    break;

	    case ARG_BA:
	    case ARG_IA:
	    if (size == 0)
		return (RSS_RPC_FORMAT_ERROR);
	    p = va_arg (vargs, char *);
	    if (io & ARG_IN) {
		if (p == NULL)
		    control |= (1 << cnt);
		else if (Add_to_buffer (b, p, size) < 0)
		    return (RSS_MALLOC_FAILED);
	    }
	    break;

	    default:
	    return (RSS_RPC_UNKNOWN_ARG_TYPE);
	}
	cnt++;
    }

    /* update the control field */
    if (!Local_endian)
	Int_byte_swap ((int *)&control, 1);
    *((int *)(b->buf + cntl_off)) = control;

    return (len);
}

/********************************************************************

    Sets function return value and output args from data in "ret_str"
    of length "len". "" and "" are respectively the argument format 
    string and the variable argument list.

********************************************************************/

static int Set_output_data (char *arg_f, 
				va_list vargs, int ret_len, char *ret_str) {
    char type_name[TYPE_SIZE], *cpt;
    int len, size, io, type;
    char *data;
    unsigned int control;

    cpt = arg_f;
    data = ret_str;
    control = *((int *)data);	/* get data endianness and stdout size */
    if (!Local_endian)
	Int_byte_swap ((int *)&control, 1);
    if ((control & 0x80000000) != Local_endian)
	Need_byteswap = 1;
    else
	Need_byteswap = 0;
    data += ALIGNED_SIZE (sizeof (int));
    while ((len = Parse_arg_format (cpt, &type, &io, &size, type_name)) > 0) {
	int cr_size;
	char *cr_p;

	cpt += len;
	if ((io & ARG_IN) == io)	/* input only arg */
	    cr_p = NULL;
	else
	    cr_p = data;
	cr_size = 0;
	switch (type) {
	    int *ip, t;
	    char *p, **pp;

	    case ARG_I:		/* function return only */
	    case ARG_P:
	    ip = va_arg (vargs, int *);
	    if (cr_p != NULL) {
		memcpy (ip, cr_p, sizeof (int));
		if (type == ARG_I && Need_byteswap)
		    Int_byte_swap (ip, 1);
		cr_size = sizeof (int);
	    }
	    break;

	    case ARG_S:
	    case ARG_V:
	    pp = va_arg (vargs, char **);
	    if (cr_p != NULL) {
		t = *((int *)cr_p);
		if (Need_byteswap)
		    Int_byte_swap (&t, 1);
		if (t == VOID_OFFSET) 
		    *pp = NULL;
		else {
		    *pp = ret_str + t;
		    if (type == ARG_V) {
			RSS_variable_size_arg *va;
			va = (RSS_variable_size_arg *)(ret_str + t);
			va->data = (char *)va + 
					sizeof (RSS_variable_size_arg);
			if (type_name[0] == '\0')
			    va->free_data = va->free_this = 0;
			else {
			    char *out;
			    int size, ret;
			    size = va->size;
			    if (Need_byteswap)
				Int_byte_swap (&size, 1);
			    ret = Deserialize_data (type_name, 
					va->data, size, 
					&out, Need_byteswap);
			    if (ret < 0)
				return (ret);
			    *pp = out;
			}
		    }
		}
		cr_size = sizeof (int);
	    }
	    break;

	    case ARG_BA:
	    case ARG_IA:
	    p = va_arg (vargs, char *);
	    if (cr_p != NULL) {
		memcpy (p, cr_p, size);
		if (type == ARG_IA && Need_byteswap)
		    Int_byte_swap ((int *)p, size / sizeof (int));
		cr_size = size;
	    }
	    break;
	}
	data += ALIGNED_SIZE (cr_size);
    }

    len = control & 0x7fffffff;		/* stdout length */
    if (len) {				/* print stdout and stderr */
	if (Get_stdout != NULL)
	    Get_stdout (ret_str + ret_len - len);
	else
	    printf ("%s", ret_str + ret_len - len);
    }

    return (0);
}

/********************************************************************

    Parses the argument format at "cpt". The results are returned in
    "type", "io" and "size". In case of user-defined type, the type
    name is return in "type_name". The function returns the number of 
    bytes consumed for the argument in the format string. It returns 
    0 if there is no argument or a negative error number.

********************************************************************/

static int Parse_arg_format (char *cpt, 
			int *type, int *io, int *size, char *type_name) {
    char *pst, *pend, *p, c, pstv;
    int t, cnt;

    *io = ARG_IN;
    *size = 0;
    pst = cpt;
    while (*pst == ' ')
	pst++;
    pend = pst;
    while (*pend != ' ' && *pend != '\0')
	pend++;
    if (pend - pst == 0)
	return (0);
    p = pst;
    while (*p != '-' && p < pend)
	p++;
    t = 0;
    pstv = *pst;
    if (p - pst == 1) {
	if (pstv == 'i')
	    t = ARG_I;
	else if (pstv == 's')
	    t = ARG_S;
	else if (pstv == 'p')
	    t = ARG_P;
	else if (pstv == 'v')
	    t = ARG_V;
    }
    else if (p - pst == 2) {
	if (pstv == 'i' && pst[1] == 'a') {
	    t = ARG_IA;
	    *size = 1;
	}
	if (pstv == 'b' && pst[1] == 'a')
	    t = ARG_BA;
    }
    type_name[0] = '\0';
    if (t == 0) {
	if (p - pst >= TYPE_SIZE)
	    return (RSS_RPC_FORMAT_ERROR);
	t = ARG_V;
	memcpy (type_name, pst, p - pst);
	type_name[p - pst] = '\0';
    }
    *type = t;
    if (p == pend)
	return (pend - cpt);
    p++;		/* *p == '-' */
    pst = p;
    while (*p != '-' && p < pend)
	p++;
    if (p == pst)
	return (RSS_RPC_FORMAT_ERROR);
    if (t == ARG_BA || t == ARG_IA) {
	cnt = sscanf (pst, "%d%c", size, &c);	/* try to read size */
	if (cnt == 1 || (cnt == 2 && c == ' '))
	    return (pend - cpt);
	if (cnt == 2 && c != '-')
	    return (RSS_RPC_FORMAT_ERROR);
	if (cnt == 2) {
	    p++;		/* *p == '-' */
	    pst = p;
	    while (*p != '-' && p < pend)
		p++;
	    if (p == pst)
		return (RSS_RPC_FORMAT_ERROR);
	}
	if (t == ARG_IA)
	    *size = *size * sizeof (int);
    }
    if (p != pend)		/* bad - */
	return (RSS_RPC_FORMAT_ERROR);
    pstv = *pst;
    if (p - pst == 1) {		/* check the io part */
	if (pstv == 'i')
	    *io = ARG_IN;
	else if (pstv == 'o')
	    *io = ARG_OUT;
	else if (pstv == 'r')
	    *io = ARG_RET;
	else
	    return (RSS_RPC_FORMAT_ERROR);
    }
    else if (p - pst == 2) {
	if (pstv == 'i' && pst[1] == 'o')
	    *io = ARG_IN | ARG_OUT;
	else
	    return (RSS_RPC_FORMAT_ERROR);
    }
    else
	return (RSS_RPC_FORMAT_ERROR);
    return (pend - cpt);
}

/***************************************************************************

    Serializes data "in" of type "type_name". The results are returned
    with "out". Returns the size of "out" on success or a negative
    error code. The pointer saved and freed latter in this function.

***************************************************************************/

static int Serialize_data (char *type_name, char *in, char **out, int *fr) {
    int ret;

    *fr = 0;
    if ((ret = Load_smi_function ()) < 0)
	return (ret);

    ret = SMIA_serialize (type_name, in, out, 0);
    if (ret > 0 && in != *out)
	*fr = 1;
    return (ret);
}

/***************************************************************************

    Deserializes data "in" of type "type_name" and length "size" bytes. The 
    results are returned with "out". Returns the size of "out" on success 
    or a negative error code. If "need_byteswap" is TRUE (non-zero), 
    byteswapping is performed after deserializing. The pointer saved and 
    freed latter in this function.

***************************************************************************/

static int Deserialize_data (char *type_name, char *in, 
				int size, char **out, int need_byteswap) {
    int ret;

    if ((ret = Load_smi_function ()) < 0)
	return (ret);

    ret = SMIA_deserialize (type_name, in, out, size);
    if (ret > 0 && in != *out &&
	Save_free_pointers (type_name, *out) < 0)
	return (RSS_MALLOC_FAILED);
    if (ret <= 0)
	return (ret);

    if (need_byteswap)
	ret = SMIA_bswap_input (type_name, *out, ret);
    return (ret);
}

/***************************************************************************

    Saves pointer "save_p", which is a struct pointer of type
    "type_name" or a normal pointer if type_name is empty, for being
    freed latter. If type_name is DEL_ALL, all saved pointers as freed.
    If type_name is DEL_HIGH_LEVEL, all pointer saved from 3 levels
    above the current level are freed.

***************************************************************************/

struct Saved_struct_p {
    char *p;		/* pointer to save/free */
    char *type_name;	/* type name */
    void *smi;		/* SMI function pointer at the time of save */
    int level;		/* call level at the time of save */
    struct Saved_struct_p *next;
};
typedef struct Saved_struct_p Saved_struct_p_t;

static int Save_free_pointers (char *type_name, char *save_p) {
    static Saved_struct_p_t *first = NULL;
    Saved_struct_p_t *p, *next;
    int del_l;

    if (type_name == DEL_ALL)
	del_l = 0;
    else if (type_name == DEL_HIGH_LEVEL)
	del_l = Call_level + 3;
    else
	del_l = -1;

    if (del_l >= 0) {		/* free saved pointers */
	Saved_struct_p_t *prev;
	p = first;
	prev = NULL;
	first = NULL;
	while (p != NULL) {
	    next = p->next;
	    if (p->level >= del_l) {	/* free this node */
		if (p->type_name[0] == '\0')
		    free (p->p);
		else {
		    void *smi = SMIA_set_smi_func (NULL);
		    SMIA_set_smi_func (p->smi);
		    SMIA_free_struct (p->type_name, p->p);
		    SMIA_set_smi_func (smi);
		}
		free (p);
		if (prev != NULL)
		    prev->next = next;
	    }
	    else {			/* keep this node */
		if (prev == NULL)
		    first = p;
		prev = p;
	    }
	    p = next;
	}
	return (0);
    }

    next = (Saved_struct_p_t *)malloc 
		(sizeof (Saved_struct_p_t) + strlen (type_name) + 1);
    if (next == NULL)
	return (-1);
    next->type_name = (char *)next + sizeof (Saved_struct_p_t);
    strcpy (next->type_name, type_name);
    if (type_name[0] != '\0')
	next->smi = SMIA_set_smi_func (NULL);
    else
	next->smi = NULL;
    next->level = Call_level;
    next->p = save_p;
    next->next = NULL;

    p = first;
    while (p != NULL && p->next != NULL)
	p = p->next;
    if (p == NULL)
	first = next;
    else
	p->next = next;
    return (0);
}

/***************************************************************************

    Loads the SMI function.. It returns 0 on success or a negative error
    code. This works on both client and server.

***************************************************************************/

static int Load_smi_function () {
    SMI_info_t *(*func_p)(char *, void *);
    int ret;

    if (Smi_func_name == NULL)
	return (0);

    ret = Get_function_pt (Smi_func_name + 1, (void **)&func_p);
    if (ret < 0)
	return (ret);

    SMIA_set_smi_func (func_p);
    return (0);
}

/********************************************************************

    Adds "data" of "len" bytes to buffer "b". The buffer is realocated
    if necessary. If data is NULL we don't cp after reallocate. Thus
    "data" of NULL must be used with caution.

********************************************************************/

static int Add_to_buffer (Buffer_t *b, void *data, int len) {
    int alen;

    alen = ALIGNED_SIZE (len);		/* aligned length */
    if (alen + b->len > b->size) {	/* we need to do malloc */
	char *cpt;
	int new_s;

	new_s = ((alen + b->len) * 3 / 2) + 1024;
	cpt = (char *)malloc (new_s);
	if (cpt == NULL)
	    return (-1);
	memcpy (cpt, b->buf, b->len);
	if (b->buf != b->st_buf)	/* not freed if b->st_buf */
	    free (b->buf);
	b->buf = cpt;
	b->size = new_s;
    }
    if (data != NULL) {
	memcpy (b->buf + b->len, data, len);
#ifdef USE_MEMORY_CHECKER
	if (alen > len)
	    memset (b->buf + b->len + len, 0, alen - len);
#endif
    }
    b->len += alen;
    return (0);
}

/********************************************************************

    Frees the dynamic buffer in "b".

********************************************************************/

static void Free_buffer (Buffer_t *b) {
    if (b->buf != b->st_buf) {
	if (b->buf != NULL)
	    free (b->buf);
	b->buf = NULL;
	b->size = b->len = 0;
    }
}

/********************************************************************

    Byte swaps an integer array "p" of size "cnt".

********************************************************************/

static void Int_byte_swap (int *p, int cnt) {
    int i;

    for (i = 0; i < cnt; i++) {
	char *cp, c;

	cp = (char *)(p + i);
	c = cp[0];
	cp[0] = cp[3];
	cp[3] = c;
	c = cp[1];
	cp[1] = cp[2];
	cp[2] = c;
    }
}

/********************************************************************

    Function call interface. A better implementation could be written
    in assembly. I have dont it for SPARC and partly done for INTEL.
    This implementation, however, is simple and portable.

********************************************************************/

static int Function_call_interface (int n_args, int *args, void *f_p) {
    int a[16], ret, i;
    int (*func_p)(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);

    for (i = 0; i < n_args; i++)
	a[i] = args[i];

    func_p = (int (*)(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int))f_p;
    ret = func_p (a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], 
		a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);
    return (ret);
}

/*************************************************************************

    Returns the accumulated stdout and stderr with "std" of size "buf_size".
    The data returned in "std" is always null-terminated and truncated by 
    "buf_size". Returns the number of bytes (null included) in "std". If 
    "std" is NULL, the existing stdout and stderr data are discarded.

*************************************************************************/

static int Get_std_msg (char *std, int buf_size) {
    static int fd = -1, in_bg = -1;
    int size;

    if (in_bg < 0)
	in_bg = RMT_is_in_backgroud ();
    if (!in_bg)
	return (0);

    if (fd == -1) {
	char fname[128];
	sprintf (fname, "/tmp/rssd.std.%d", (int)getpid ());
	if ((fd = open (fname, O_RDWR | O_CREAT, 0666)) < 0 ||
	    dup2 (fd, STDOUT_FILENO) != STDOUT_FILENO ||
	    dup2 (fd, STDERR_FILENO) != STDERR_FILENO) {
	    MISC_log ("Failed in setting RPC STDIO ports\n");
	    close (fd);
	    fd = -2;
	}
	MISC_unlink (fname);
    }

    if (fd >= 0)
	fflush (stdout);
    if (std == NULL) {
	if (fd >= 0) {
	    lseek (fd, 0, SEEK_SET);
	    ftruncate (fd, 0);
	}
	return (0);
    }
    std[0] = '\0';
    if (buf_size <= 0)
	return (0);
    size = lseek (fd, 0, SEEK_CUR);
    if (size <= 0)
	return (0);
    if (size >= buf_size)
	size = buf_size - 1;
    lseek (fd, 0, SEEK_SET);
    size = read (fd, std, size);
    if (size <= 0)
	return (0);
    std[size] = '\0';
    return (size + 1);
}

#ifdef NO_SMI

/**************************************************************************

   To support the small libinfr.

**************************************************************************/

int SMIA_bswap_input (char *type, void *data, int data_len) {
    return (RSS_NOT_SUPPORTED);
}
int SMIA_deserialize (char *type, char *serial_data, 
				char **c_data, int serial_data_size) {
    return (RSS_NOT_SUPPORTED);
}
void *SMIA_set_smi_func (SMI_info_t *(*smi_get_info)(char *, void *)) {
}
int SMIA_free_struct (char *type, char *c_data) {
    return (RSS_NOT_SUPPORTED);
}
int SMIA_serialize (char *type, void *c_data, 
				char **serial_data, int c_data_size) {
    return (RSS_NOT_SUPPORTED);
}
#endif

