/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2005/04/15 19:36:06 $
 * $Id: smipp_main.c,v 1.10 2005/04/15 19:36:06 jing Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>

#include <smi.h>
#include "smipp_def.h"


#define CPP_OPTION_SIZE 256

char Prog_name[MIPP_NAME_SIZE];

static char Output_fname[MIPP_NAME_SIZE];
static char tmp_cppfile[MIPP_NAME_SIZE] = "";
static char Cpp_options[CPP_OPTION_SIZE];
static char Cpp_flags[CPP_OPTION_SIZE];
static char Cpp_name[MIPP_NAME_SIZE];
static char Data_file_name[MIPP_NAME_SIZE];

static FILE *Out_fl;
static int Need_byte_swap_func = 0;	/* need to create function 
					   Byte_swap_data */
static int Pos_of_byte_swap_data_func_template = 0;
static int Size_of_byte_swap_data_func_template = 0;

int SMIM_debug = 0;

typedef struct {		/* write buffer struct */
    int size;			/* buffer size */
    int n_used;			/* number of valid bytes */
    int cnt;			/* items (e.g. integer) count */
    int n;			/* number of bytes added in this call */
    int n_in_current_line;	/* number of bytes in the current line */
    char *line_return_pattern;	/* line return patter added */
    char *buf;			/* buffer pointer */
} Buffer_t;


static void Print_usage ();
static void Apply_c_preprocessor (char *cmd, char *fname);
static void Process_a_file (char *cmd, char *fname);
static void Generate_smi_func_code (Smi_info_t *smi, int off,
		Buffer_t *ints_b, Buffer_t *names_b, Buffer_t *b);
static void Generate_get_info_code ();
static void Generate_byte_swap_code ();
static void Write_tmp_file (char *buf, int size);
static int Read_options (int argc, char **argv);
static char *Get_buffer (char *cpt, char **buf, int inc);
static void Store_data (Smi_info_t *smi);
static char *Add_byte_swap (char *str, Smi_info_t *smi);
static void Init_buffer (Buffer_t *buf);
static void Write_buffer_to_file (Buffer_t *buf, FILE *fl);
static void Bprintf (Buffer_t *buf, char *format, ...);
static void Generate_smi_init ();
static void Generate_type_id_code ();
static void Sort_type (int n, Smi_struct_t **ra);


/****************************************************************

****************************************************************/

#define CMD_SIZE 512

int main (int argc, char **argv) {
    int optind;
    char cmd[CMD_SIZE], *std_opt;
    int cnt;

    strncpy (Prog_name, argv[0], MIPP_NAME_SIZE);
    Prog_name [MIPP_NAME_SIZE - 1] = '\0';

    optind = Read_options (argc, argv);
    if (Cpp_name[0] == '\0')
	SMIP_error (-1, "C preprocessor name not specified\n");

    if (strlen (Cpp_flags) > 0)
	std_opt = Cpp_flags;
    else if (strcmp (Cpp_name, "gcc") == 0)
	std_opt = "-E -C";
    else if (strcmp (Cpp_name, "c89") == 0 ||
		strcmp (Cpp_name, "cc") == 0)
	std_opt = "-E -C";
    else
	std_opt = "";

    cnt = 0;
    while (optind < argc) {
	static char prev_tmp_cppfile[MIPP_NAME_SIZE] = "";
	char *cpt;
	int len;

	cpt = argv[optind];
	optind++;
	len = strlen (cpt);
	if (len >= 3 && (strcmp (cpt + len - 2, ".m") == 0 ||
		strcmp (cpt + len - 2, ".h") == 0)) {
	    printf ("process file %s - %s\n", cpt, Prog_name);
	    memcpy (Output_fname, cpt, len - 2);
	    Output_fname[len - 2] = '\0';
	    strcat (Output_fname, ".c");
	    memcpy (tmp_cppfile, cpt, len - 2);
	    tmp_cppfile[len - 2] = '\0';
	    strcat (tmp_cppfile, ".i");
	    sprintf (cmd, "rm -f %s %s; %s %s %s ", 
		prev_tmp_cppfile, tmp_cppfile, Cpp_name, std_opt, Cpp_options);
	    Process_a_file (cmd, cpt);
	    strcpy (prev_tmp_cppfile, tmp_cppfile);
	    cnt++;
	}
	else
	    printf ("file %s not processed - %s\n", cpt, Prog_name);
    }

    if (!SMIM_debug)
	unlink (tmp_cppfile);

    if (cnt == 0)
	SMIP_error (-1, "0 file processed\n");

    exit (0);
}

/**************************************************************************

    Processes a .m file.

**************************************************************************/

static void Process_a_file (char *cmd, char *fname) {
    int fd, size, off_cnt;
    char *in_buf, *out_buf, *func_temp;
    int tkcnt, *tks;
    Smi_struct_t *s;
    Buffer_t ints_b, names_b, offs_b, func_b;

    Apply_c_preprocessor (cmd, fname);

    /* open and read the cpp processed file */
    fd = open (tmp_cppfile, O_RDONLY);
    if (fd < 0)
	SMIP_error (-1, "open (%s) failed\n", tmp_cppfile);
    size = lseek (fd, 0, SEEK_END);
    lseek (fd, 0, SEEK_SET);
    in_buf = SMIM_malloc (size);
    if (read (fd, in_buf, size) != size)
	SMIP_error (-1, "read (size %d) failed\n", size);
    close (fd);

    size = SMIP_tokenize_text (in_buf, size, &out_buf, &tks, &tkcnt);
    free (in_buf);

    SMIT_init (out_buf, tks, tkcnt);

    if (SMIM_debug)
	Write_tmp_file (out_buf, size);

    /* generate the C file */
    fprintf (Out_fl, "\n\n\n/* The following is generated by smipp */\n\n");
    fprintf (Out_fl, "#include <smi.h>\n\n");
    fprintf (Out_fl, "static int Need_byte_swap_data = 0;\n\n");
    Pos_of_byte_swap_data_func_template = ftell (Out_fl);
    func_temp = "static int Byte_swap_data (void *data, char *type);";
    Size_of_byte_swap_data_func_template = strlen (func_temp);

    fprintf (Out_fl, "%s\n\n", func_temp);
    Need_byte_swap_func = 0;

    Init_buffer (&ints_b);
    Init_buffer (&names_b);
    Init_buffer (&offs_b);
    Init_buffer (&func_b);
    Bprintf (&names_b, "static const char All_names[] = \n\"");
    Bprintf (&ints_b, "static const int All_ints[] = {\n");
    Bprintf (&offs_b, "static const int All_offs[] = {\n");
    names_b.line_return_pattern = "\\\n";
    ints_b.line_return_pattern = "\n";
    offs_b.line_return_pattern = "\n";

    s = SMIP_get_smis ();
    off_cnt = 0;
    while (s != NULL) {
	Smi_info_t *smi;
	char buf[512];
	Smi_struct_t *t;

	SMIT_find_typedef (s->name, buf, 512);
	s->real_name = SMIM_malloc (strlen (buf) + 1);
	strcpy (s->real_name, buf);

	t = SMIP_get_smis ();	/* skip duplicated structures */
	while (t != s) {
	    if (strcmp (t->real_name, s->real_name) == 0)
		break;
	    t = t->next;
	}
	if (t == s) {		/* not a duplicated strust */
	    if ((smi = SMIT_get_smi (s->real_name)) != NULL) {
		Bprintf (&offs_b, " %d, %d,", names_b.cnt, ints_b.cnt);
		Generate_smi_func_code (smi, off_cnt,
					&ints_b, &names_b, &func_b);
		off_cnt += 2;
		Store_data (smi);
	    }
	    else {
		if (SMIM_debug)
		    printf ("\"%s\" not found or not a struct - %s\n",
						s->real_name, Prog_name);
		s->real_name[0] = '\0';	/* No SMI. May be a primitive type */
	    }
	}
	s = s->next;
    }
    names_b.line_return_pattern = NULL;
    ints_b.line_return_pattern = NULL;
    offs_b.line_return_pattern = NULL;
    Bprintf (&names_b, "\"\n;\n\n");
    if (ints_b.n_used > 0)		/* remove the last ',' */
	ints_b.buf[ints_b.n_used - 1] = ' ';
    Bprintf (&ints_b, "\n};\n\n");
    if (offs_b.n_used > 0)		/* remove the last ',' */
	offs_b.buf[offs_b.n_used - 1] = ' ';
    Bprintf (&offs_b, "\n};\n\n");

/*    fprintf (Out_fl, "static int N_types = %d;\n\n", off_cnt / 2); */
    Write_buffer_to_file (&names_b, Out_fl);
    Write_buffer_to_file (&ints_b, Out_fl);
    Write_buffer_to_file (&offs_b, Out_fl);
    Generate_smi_init ();
    Write_buffer_to_file (&func_b, Out_fl);

    Generate_get_info_code ();
    Generate_type_id_code ();
    Generate_byte_swap_code ();

    if (out_buf != NULL)
	free (out_buf);
    if (tks != NULL)
	free (tks);
    SMIP_free ();
    SMIT_free ();
    fclose (Out_fl);
}

/**************************************************************************

    Generate the C code for function templates. "n_types" is the total 
    number of types.

**************************************************************************/

static void Generate_get_info_code () {
    Smi_struct_t *s;
    Smi_struct_t **types;
    int cnt, i;

    fprintf (Out_fl, "SMI_info_t *%s (char *type_name, void *data) {\n",
					SMIP_get_function_name ());

    s = SMIP_get_smis ();
    cnt = 0;
    while (s != NULL) {			/* find number of struct names */
	if (s->real_name[0] != '\0')
	    cnt++;
	s = s->next;
    }

    types = (Smi_struct_t **)SMIM_malloc (cnt * sizeof (Smi_struct_t *));
    s = SMIP_get_smis ();
    cnt = 0;
    while (s != NULL) {
	if (s->real_name[0] != '\0') {
	    types[cnt] = s;
	    cnt++;
	}
	s = s->next;
    }
    Sort_type (cnt, types);

    fprintf (Out_fl, 
"\
    static struct {\n\
	SMI_info_t *(*func) (void *);\n\
	char *name;\n\
    } type_list [] = {\n\
");			/* end of fprintf */

    for (i = 0; i < cnt; i++) {
	int under_score;

	under_score = 0;
	if (strncmp (types[i]->real_name, "struct ", 7) == 0) {
	    under_score = 1;
	    types[i]->real_name[6] = '_';
	}
	fprintf (Out_fl, "	{ %s_info_, \"%s\"},\n", 
					types[i]->real_name, types[i]->name);
	if (under_score)
	    types[i]->real_name[6] = ' ';
    }
    fprintf (Out_fl, "    };\n");
    fprintf (Out_fl, "    static int n_types = %d;\n", cnt);
    fprintf (Out_fl, "    int st, end, ind, i;\n\n");

    fprintf (Out_fl, "    if (type_name == NULL) {\n");
    fprintf (Out_fl, "\tif (data == NULL)\n");
    fprintf (Out_fl, "\t    Need_byte_swap_data = 0;\n");
    fprintf (Out_fl, "\telse\n");
    fprintf (Out_fl, "\t    Need_byte_swap_data = 1;\n");
    fprintf (Out_fl, "\treturn (NULL);\n");
    fprintf (Out_fl, "    }\n");

    fprintf (Out_fl, 
"\
    /* binary search */\n\
    st = 0;\n\
    end = n_types - 1;\n\
    ind = -1;\n\
    while (1) {\n\
	i = (st + end) >> 1;\n\
	if (st == i) {\n\
	    if (strcmp (type_name, type_list[st].name) == 0)\n\
		ind = st;\n\
	    else if (strcmp (type_name, type_list[end].name) == 0)\n\
		ind = end;\n\
	    break;\n\
	}\n\
	if (strcmp (type_name, type_list[i].name) <= 0)\n\
	    end = i;\n\
	else\n\
	    st = i;\n\
    }\n\
    if (ind < 0)\n\
	return (NULL);\n\
    else \n\
	return (type_list[ind].func (data));\n\
}\n\
\n\
");			/* end of fprintf */
    free (types);
}

/**************************************************************************

    Generate the C code for a struct and stored in buffer "b".

**************************************************************************/

static void Generate_smi_func_code (Smi_info_t *smi, int off,
		Buffer_t *ints_b, Buffer_t *names_b, Buffer_t *b) {
    int under_score, i;

    Bprintf (b, 
	"%c*** The next function is generated using code in file\n", '/');
    Bprintf (b, 
	"     %s started at line %d ***%c\n\n",
			 smi->src_name, smi->src_line, '/');

    Bprintf (names_b, "%s\\0", smi->name);
    names_b->cnt += names_b->n - 1;
    Bprintf (ints_b, " sizeof (%s), %d, %d,", 
				smi->name, smi->n_fs, smi->n_vsfs);
    ints_b->cnt += 3;

    under_score = 0;
    if (strncmp (smi->name, "struct ", 7) == 0) {
	under_score = 1;
	smi->name[6] = '_';
    }
    Bprintf (b, "static SMI_info_t *%s_info_ (void *dt) {\n", smi->name);
    if (under_score)
	smi->name[6] = ' ';
    Bprintf (b, "    static SMI_info_t *mi = NULL;\n");
    Bprintf (b, "    static int off = %d;\n", off);
    Bprintf (b, "    SMI_field_t *fmi;\n");
    for (i = 0; i < smi->n_fs; i++) {
	Smi_field_t *fld;
	char *type;
	fld = smi->fields + i;
	type = fld->type;
	if (strcmp (type, "enum") == 0)
	    type = "int";
	Bprintf (names_b, "%s\\0%s\\0", fld->name, type);
	names_b->cnt += names_b->n - 2;
	Bprintf (ints_b, " %d, sizeof (%s),", fld->n_items, type);
	ints_b->cnt += 2;
    }
    Bprintf (b, "    %s *tmp = NULL;\n", smi->name);
    if (smi->need_data)
	Bprintf (b, "    %s *data = (%s *)dt;\n", smi->name, smi->name);
    Bprintf (b, "\n");

    Bprintf (b, "    if (mi == NULL)\n");
    Bprintf (b, "\tmi = Smi_init (off);\n\n");

    Bprintf (b, "    fmi = mi->fields;\n");

    if (smi->need_data) {
	Bprintf (b, "    if (dt == NULL)\n");
	Bprintf (b, "\treturn (mi);\n\n");
    }
    if (smi->vsize != NULL)
	Bprintf (b, "    mi->size = %s;\n", 
				Add_byte_swap (smi->vsize, smi));

    for (i = 0; i < smi->n_fs; i++) {
	if (smi->fields[i].ni_exp != NULL)
	    Bprintf (b, 
		"    fmi[%d].n_items = %s;\n", i, 
				Add_byte_swap (smi->fields[i].ni_exp, smi));
	if (smi->fields[i].offset_exp != NULL)
	    Bprintf (b, 
		"    fmi[%d].offset = %s;\n", i, 
			Add_byte_swap (smi->fields[i].offset_exp, smi));
	else
	    Bprintf (b, 
	"    fmi[%d].offset = (char *)&(tmp->%s) - (char *)tmp;\n",
			 i, smi->fields[i].name);
    }

    Bprintf (b, "\n");
    Bprintf (b, "    return (mi);\n");
    Bprintf (b, "}\n\n\n");
}

/**************************************************************************

    Replacing "data->field" int "str" by (Byte_swap (&(data->field), type))
    for byte swapping the data before evaluating field value dependent SMI.
    Returns the modified string.

**************************************************************************/

static char *Add_byte_swap (char *str, Smi_info_t *smi) {
    static char buf[1024];
    char *tk[64], *in, *out;
    int tk_len[64], cnt, bsize, size_inc, len, i;

    if (str == NULL)
	return (0);
    bsize = 1024 - strlen (str);
    size_inc = 32;
    in = str;
    out = buf;
    cnt = SMIP_get_next_tokens (64, str, tk, tk_len);
    for (i = 0; i < cnt; i++) {
	if (tk_len[i] == 4 && strncmp (tk[i], "data", 4) == 0 &&
		i + 3 < cnt && *(tk[i + 1]) == '-' && *(tk[i + 2]) == '>') {
	    int f;
	    for (f = 0; f < smi->n_fs; f++) {
		if (strncmp (tk[i + 3], 
				smi->fields[f].name, tk_len[i + 3]) == 0 &&
		    strlen (smi->fields[f].name) == tk_len[i + 3])
		    break;
	    }
	    if (f < smi->n_fs) {
		if (bsize <= size_inc)
		    SMIP_error (-1, "expression (%s) too long\n", str);
		strcpy (out, "(Byte_swap_data (&(");
		out += strlen (out);
		len = (tk[i + 3] + tk_len[i + 3]) - tk[i];
		memcpy (out, tk[i], len);
		out += len;
		sprintf (out, "), \"%s\"))", smi->fields[f].type);
		out += strlen (out);
		in += len;
		bsize -= size_inc;
		i += 3;
		Need_byte_swap_func = 1;
		continue;
	    }
	}
	len = (tk[i] + tk_len[i]) - in;
	memcpy (out, in, len);
	in += len;
	out += len;
    }
    *out = '\0';
    return (buf);
}

/**************************************************************************

    Writes the SMI to a file.

**************************************************************************/

static void Store_data (Smi_info_t *smi) {
    static FILE *fl = NULL;
    char *buf, *cpt;
    SMI_data_info_t *info;
    int i;

    if (Data_file_name[0] == '\0')
	return;
    if (fl == NULL) {
	fl = fopen (Data_file_name, "w");
	if (fl == NULL)
	    SMIP_error (-1, "open data file (%s) failed\n", Data_file_name);
    }

    cpt = Get_buffer (NULL, &buf, sizeof (SMI_data_info_t) + 
		sizeof (SMI_data_field_t) * (smi->n_fs - 1));
    info = (SMI_data_info_t *)buf;
    info->name_off = cpt - buf;
    strcpy (cpt, smi->name);
    cpt = Get_buffer (cpt, &buf, strlen (smi->name) + 1);
    info->n_fields = smi->n_fs;
    info->n_vsfs = smi->n_vsfs;
    for (i = 0; i < smi->n_fs; i++) {
	info->fields[i].name_off = cpt - buf;
	strcpy (cpt, smi->fields[i].name);
	cpt = Get_buffer (cpt, &buf, strlen (cpt) + 1);
	info->fields[i].type_off = cpt - buf;
	strcpy (cpt, smi->fields[i].type);
	cpt = Get_buffer (cpt, &buf, strlen (cpt) + 1);
	info->fields[i].n_items = smi->fields[i].n_items;
    }
    info->size = cpt - buf;

    if (fwrite (buf, 1, cpt - buf, fl) != cpt - buf)
	SMIP_error (-1, 
		"write %s (size %d) failed\n", Data_file_name, cpt - buf);
}

/**************************************************************************

    Gets the buffer needed. "cpt" is the current pointer and "inc" is the 
    intended increment. "buf" returns the buffer pointer. Returns the 
    incremented pointer.

**************************************************************************/

#define TMP_BUF_INC 4096

static char *Get_buffer (char *cpt, char **buf, int inc) {
    static char *buffer = NULL;
    static int total = 0;

    if (cpt == NULL)
	cpt = buffer;

    if (buffer == NULL || cpt - buffer + inc > total) {
	int new_size;
	char *b;
	new_size = total + TMP_BUF_INC;
	b = SMIM_malloc (new_size);
	cpt = b + (cpt - buffer);
	if (buffer != NULL) {
	    memcpy (b, buffer, total);
	    free (buffer);
	}
	buffer = b;
	total = new_size;
    }
    *buf = buffer;
    return (cpt + inc);
}

/**************************************************************************

    Creates the original C file from the .m file and applies the C
    preprocessor to the C file. For this moment, we make a copy. We may
    want to open the file later to get info and modify it.

**************************************************************************/

static void Apply_c_preprocessor (char *cmd, char *fname) {
    int cmd_len, fd, size;
    char *in_buf;

    /* open the m file */
    fd = open (fname, O_RDONLY);
    if (fd < 0)
	SMIP_error (-1, "open (%s) failed\n", fname);
    size = lseek (fd, 0, SEEK_END);
    lseek (fd, 0, SEEK_SET);
    in_buf = SMIM_malloc (size);
    if (read (fd, in_buf, size) != size)
	SMIP_error (-1, "read (size %d) failed\n", size);
    close (fd);

    /* open output file */
    Out_fl = fopen (Output_fname, "w");
    if (Out_fl == NULL)
	SMIP_error (-1, "open (%s) failed\n", Output_fname);
    if (fwrite (in_buf, 1, size, Out_fl) != size)
	SMIP_error (-1, "write %s (size %d) failed\n", Output_fname, size);

    fclose (Out_fl);		/* close for cpp command to work */
    cmd_len = strlen (cmd);
    if (cmd_len + strlen (Output_fname) >= CMD_SIZE)
	SMIP_error (-1, "command line too long (file %s)\n", fname);
    sprintf (cmd + cmd_len, "%s > %s", Output_fname, tmp_cppfile);
    if (system (cmd) != 0)
	SMIP_error (-1, "C preprocessor (%s) failed\n", cmd);
    cmd[cmd_len] = '\0';			/* for next file */
    Out_fl = fopen (Output_fname, "r+");	/* reopen */
    fseek (Out_fl, 0, SEEK_END);
    free (in_buf);
}

/**************************************************************************

    Memory allocation of size "size".

**************************************************************************/

char *SMIM_malloc (int size) {
    char *p;
    p = malloc (size);
    if (p == NULL)
	SMIP_error (-1, "malloc (size %d) failed\n", size);
    return (p);
}

/**************************************************************************

    Description: This function reads command line arguments.

    Inputs:	argc - number of command arguments
		argv - the list of command arguments

    Return:	It returns 0 on success or -1 on failure.

**************************************************************************/

static int Read_options (int argc, char **argv) {
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c;                  /* used by getopt */

    strcpy (Cpp_name, "gcc");
    Cpp_options[0] = '\0';
    Cpp_flags[0] = '\0';
    Data_file_name[0] = '\0';
    SMIM_debug = 0;
    while ((c = getopt (argc, argv, "htc:o:d:f:v?")) != EOF) {
	switch (c) {

	    case 'c':
		if (sscanf (optarg, "%s", Cpp_name) != 1)
		    Print_usage ();
		break;

	    case 'd':
		if (sscanf (optarg, "%s", Data_file_name) != 1)
		    Print_usage ();
		break;

	    case 'o':
		strncpy (Cpp_options, optarg, CPP_OPTION_SIZE);
		Cpp_options[CPP_OPTION_SIZE - 1] = '\0';
		break;

	    case 'f':
		strncpy (Cpp_flags, optarg, CPP_OPTION_SIZE);
		Cpp_flags[CPP_OPTION_SIZE - 1] = '\0';
		break;

	    case 't':
		SMIM_debug = 1;
		break;

	    case 'h':
	    case '?':
		Print_usage (argv);
		break;
	}
    }

    return (optind);
}

/**************************************************************************

    Description: This function prints the usage message and then terminates
		the program.

**************************************************************************/

static void Print_usage () {

    printf ("Usage: smipp [options] file_names\n");
    printf ("       preprocessor for struct meta info\n");
    printf ("       options:\n");
    printf ("       -c cpp_name (alternative preprocessor to gcc)\n");
    printf ("       -t (runs in debug mode: intermediate files are not removed)\n");
    printf ("       -o cpp_options (preprocessor options)\n");
    printf ("       -f cpp_flags (alternative preprocessor flags to \"-E -C\")\n");
    printf ("       -d data_file_name (dumps data to the named file)\n");
    printf ("       example: smipp -t -c gcc -o \"-I../include -I.\" t.m\n");
    printf ("\n");

    exit (0);
}

/**************************************************************************

    Writes data to a tmp file.

**************************************************************************/

static void Write_tmp_file (char *buf, int size) {
    int fd, i;

    for (i = 0; i < size; i++)
	if (buf[i] == '\0')
	    buf[i] = '\n';

    fd = open ("tmp_file", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0)
	SMIP_error (-1, "open (%s) failed\n", "tmp_file");
    write (fd, buf, size);
    close (fd);

    for (i = 0; i < size; i++)
	if (buf[i] == '\n')
	    buf[i] = '\0';
}

/****************************************************************************

    Formats and writes to buffer "buf". Backslash and line return is added 
    if the line is too long.

****************************************************************************/

#define MAX_ONE_TIME_SIZE 256
#define INIT_SIZE 4096

static void Bprintf (Buffer_t *buf, char *format, ...) {
    va_list vl;
    char *p;
    int n;

    if (buf->n_used + MAX_ONE_TIME_SIZE > buf->size) {
	int new_size;

	if (buf->size == 0)
	    new_size = INIT_SIZE;
	else
	    new_size = buf->size * 2;
	p = SMIM_malloc (new_size);
	if (buf->size > 0) {
	    memcpy (p, buf->buf, buf->n_used);
	    free (buf->buf);
	}
	buf->size = new_size;
	buf->buf = p;
    }

    va_start (vl, format);
    p = buf->buf + buf->n_used;
    n = vsprintf (p, format, vl);
    buf->n = n;
    if (p[n - 1] == '\n')
	buf->n_in_current_line = 0;
    else
	buf->n_in_current_line += n;
    if (buf->line_return_pattern != NULL && buf->n_in_current_line > 72) {
	int nadd = strlen (buf->line_return_pattern);
	memcpy (p + n, buf->line_return_pattern, nadd);
	n += nadd;
	buf->n_in_current_line = 0;
    }
    buf->n_used += n;
    va_end (vl);
}

/****************************************************************************

    Writes contents in "buf" to file "fl". It frees the buffer.

****************************************************************************/

static void Write_buffer_to_file (Buffer_t *buf, FILE *fl) {

    fflush (fl);
    if (fwrite (buf->buf, 1, buf->n_used, fl) != buf->n_used)
	SMIP_error (-1, "write buffer failed\n");
    if (buf->size > 0) {
	free (buf->buf);
	buf->size = buf->n_used = buf->cnt = 0;
    }
    fflush (fl);
}

/****************************************************************************

    Initializes buffer "buf".

****************************************************************************/

static void Init_buffer (Buffer_t *buf) {

    buf->size = buf->n_used = buf->cnt = buf->n_in_current_line = 0;
    buf->line_return_pattern = NULL;
}

/****************************************************************************

    Generates function "Smi_init".

****************************************************************************/

static void Generate_smi_init () {

    fprintf (Out_fl, "%s", 
"\
/*** Initializes the SMI struct at offset \"off\" */\n\
\n\
static SMI_info_t *Smi_init (int off) {\n\
    SMI_info_t *mi;\n\
    const char *name;\n\
    const int *ip;\n\
    int n_fields, i;\n\
    SMI_field_t *flds;\n\
\n\
    name = All_names + All_offs[off];\n\
    ip = All_ints + All_offs[off + 1];\n\
    n_fields = ip[1];\n\
    while ((mi = (SMI_info_t *)malloc \n\
	(sizeof (SMI_info_t) + n_fields * sizeof (SMI_field_t))) == NULL)\n\
	sleep (1);\n\
    mi->name = (char *)name;\n\
    name += strlen (name) + 1;\n\
    mi->size = *ip;\n\
    ip++;\n\
    mi->n_fields = n_fields;\n\
    ip++;\n\
    mi->n_vsfs = *ip;\n\
    ip++;\n\
    flds = (SMI_field_t *)((char *)mi + sizeof (SMI_info_t));\n\
    mi->fields = flds;\n\
    mi->ci = NULL;\n\
\n\
    for (i = 0; i < n_fields; i++) {\n\
	flds[i].name = (char *)name;\n\
	name += strlen (name) + 1;\n\
	flds[i].type = (char *)name;\n\
	name += strlen (name) + 1;\n\
	flds[i].n_items = *ip;\n\
	ip++;\n\
	flds[i].size = *ip;\n\
	ip++;\n\
	flds[i].offset = 0;\n\
	flds[i].ci = NULL;\n\
    }\n\
    return (mi);\n\
}\n\
\n\
"
    );			/* end of fprintf */
}

/**************************************************************************

    Create C code for functions "Byte_swap_data" and 
    "SMI_get_info_set_byte_swap".

**************************************************************************/

static void Generate_byte_swap_code () {

    if (!Need_byte_swap_func) {		/* erase the function template */
	int i;
	fseek (Out_fl, Pos_of_byte_swap_data_func_template, SEEK_SET);
	for (i = 0; i < Size_of_byte_swap_data_func_template; i++)
	    fprintf (Out_fl, " ");
	return;
    }

    fprintf (Out_fl, "%s", 
"\
static int Byte_swap_data (void *data, char *type) {\n\
    char *t;\n\
    short s;\n\
    int size, r;\n\
\n\
    t = type;\n\
    if (strncmp (t, \"unsigned \", 9) == 0)\n\
	t += 9;\n\
\n\
    if (strcmp (t, \"char\") == 0)\n\
	size = 1;\n\
    else if (strcmp (t, \"short\") == 0 ||\n\
	     strncmp (t, \"short \", 6) == 0)\n\
	size = 2;\n\
    else\n\
	size = 4;\n\
\n\
    if (size == 1)\n\
	r = *((char *)data);\n\
    else if (size == 2) {\n\
	s = *((short *)data);\n\
	if (Need_byte_swap_data)\n\
	    s = ((s & 0xff) << 8) | ((s >> 8) & 0xff);\n\
	r = s;\n\
    }\n\
    else {\n\
	r = *((int *)data);\n\
	if (Need_byte_swap_data)\n\
	    r = ((r & 0xff) << 24) | ((r & 0xff00) << 8) | \n\
			((r >> 8) & 0xff00) | ((r >> 24) & 0xff);\n\
    }\n\
    return (r);\n\
}\n\
\n\
"
    );			/* end of fprintf */
}

/**************************************************************************

    Generates the C code for functions deal with major and minor IDs.

**************************************************************************/

static void Generate_type_id_code () {
    Smi_struct_t *s, *a, **ids;
    int cnt, n_ids, i, c_cnt;

    n_ids = 0;			/* find how many id items */
    s = SMIP_get_smis ();
    while (s != NULL) {
	if (s->real_name[0] != '\0') {
	    a = s;
	    while (a != NULL) {
		if (a->major != NULL)
		    n_ids++;
		a = a->next_id;
	    }
	}
	s = s->next;
    }
    ids = (Smi_struct_t **)SMIM_malloc (n_ids * sizeof (Smi_struct_t *));

    s = SMIP_get_smis ();
    cnt = 0;
    while (s != NULL) {
	if (s->real_name[0] != '\0') {
	    a = s;
	    while (a != NULL) {
		if (a->major != NULL) {
		    ids[cnt] = a;
		    cnt++;
		}
		a = a->next_id;
	    }
	}
	s = s->next;
    }

    fprintf (Out_fl, "static int N_id_types = %d;\n\n", cnt);

    fprintf (Out_fl, "static const int Majors[] = {\n");
    c_cnt = 0;
    for (i = 0; i < cnt; i++) {
	if (ids[i]->major != NULL)
	    c_cnt += fprintf (Out_fl, " %s,", ids[i]->major);
	else
	    c_cnt += fprintf (Out_fl, " -1,");
	if (c_cnt > 72) {
	    fprintf (Out_fl, "\n");
	    c_cnt = 0;
	}
    }
    fprintf (Out_fl, "};\n\n");
    fprintf (Out_fl, "static const int Minors[] = {\n");
    c_cnt = 0;
    for (i = 0; i < cnt; i++) {
	if (ids[i]->minor != NULL)
	    c_cnt += fprintf (Out_fl, " %s,", ids[i]->minor);
	else
	    c_cnt += fprintf (Out_fl, " -1,");
	if (c_cnt > 72) {
	    fprintf (Out_fl, "\n");
	    c_cnt = 0;
	}
    }
    fprintf (Out_fl, "};\n\n");
    fprintf (Out_fl, "static const char *Id_type_names[] = {\n");
    c_cnt = 0;
    for (i = 0; i < cnt; i++) {
	c_cnt += fprintf (Out_fl, " \"%s\",", ids[i]->name);
	if (c_cnt > 72) {
	    fprintf (Out_fl, "\n");
	    c_cnt = 0;
	}
    }
    fprintf (Out_fl, "};\n\n");
    free (ids);

    /* function SMI_get_info_type_by_id */
    fprintf (Out_fl, 
	"char *%s_type_by_id (int major, int minor) {", 
						SMIP_get_function_name ());
    fprintf (Out_fl, "%s", 
"\
    int ind, i;\n\
\n\
    ind = -1;\n\
    for (i = 0; i < N_id_types; i++) {\n\
	if (Majors[i] == major) {\n\
	    if (minor >= 0 && Minors[i] == minor)\n\
		return ((char *)Id_type_names[i]);\n\
	    if (Minors[i] < 0 && ind < 0) {\n\
		ind = i;\n\
		if (minor < 0)\n\
		    break;\n\
	    }\n\
	}\n\
    }\n\
    if (ind >= 0)\n\
	return ((char *)Id_type_names[ind]);\n\
    return (NULL);\n\
}\n\
\n\
");			/* end of fprintf */

    /* function SMI_get_info_type_by_index */
    fprintf (Out_fl, 
	"int %s_get_all_ids (int **major, int **minor, char ***type) {", 
						SMIP_get_function_name ());
    fprintf (Out_fl, "%s", 
"\
    *major = (int *)Majors;\n\
    *minor = (int *)Minors;\n\
    *type = (char **)Id_type_names;\n\
    return (N_id_types);\n\
}\n\
\n\
");			/* end of fprintf */
}

/********************************************************************
			
    Sort type name array "ra" of size "n". The sort is in-place.

********************************************************************/

static void Sort_type (int n, Smi_struct_t **ra) {
    int l, j, ir, i;
    Smi_struct_t *rra;				/* type dependent */

    if (n <= 1)
	return;
    ra--;
    l = (n >> 1) + 1;
    ir = n;
    for (;;) {
	if (l > 1)
	    rra = ra[--l];
	else {
	    rra = ra[ir];
	    ra[ir] = ra[1];
	    if (--ir == 1) {
		ra[1] = rra;
		return;
	    }
	}
	i = l;
	j = l << 1;
	while (j <= ir) {
	    if (j < ir && strcmp (ra[j]->name, ra[j + 1]->name) < 0)
		++j;				/* type dependent */
	    if (strcmp (rra->name, ra[j]->name) < 0) {	/* type dependent */
		ra[i] = ra[j];
		j += (i = j);
	    }
	    else
		j = ir + 1;
	}
	ra[i] = rra;
    }
}







