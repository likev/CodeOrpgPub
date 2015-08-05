
/******************************************************************

    ftc is a tool that helps porting FORTRAN programs. This module 
    contains file related functions.

******************************************************************/

/* 
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2010/10/26 21:04:26 $
 * $Id: ftc_file.c,v 1.1 2010/10/26 21:04:26 jing Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <infr.h>

#include "ftc_def.h"

#define MAX_N_FILES 256

typedef struct {
    char *name;		/* file name */
    FILE *fd;		/* file handle */
    int n_lines;	/* number of lines */
    int *loffs;		/* offset of each line */
    char *text;		/* pointer to the beging of the file */
} Open_file_t;

static Open_file_t Open_files[MAX_N_FILES];
static int N_open_files = 0;

/******************************************************************

    Returns the file name of open file "file".

******************************************************************/

char *FF_file_name (int file) {
    if (file < 0 || file >= N_open_files)
	return (NULL);
    return (Open_files[file].name);
}

/******************************************************************

    Opens file "name" and builds the line structure. Returns a file
    handle. If sizep is not NULL, file size + 1 is returned.

******************************************************************/

int FF_open_file (char *name, int *sizep, int *nlinesp) {
    FILE *fd;
    char *buf, *p;
    int size, cnt;
    Open_file_t *of;

    fd = fopen (name, "r");
    if (fd == NULL) {
	fprintf (stderr, "Could not open file %s\n", name);
	exit (1);
    }
    if (fseek (fd, 0, SEEK_END) < 0 ||
	(size = ftell (fd)) < 0 ||
	fseek (fd, 0, SEEK_SET) < 0) {
	fprintf (stderr, "Failed in finding file (%s) size\n", name);
	exit (1);
    }

    buf = MISC_malloc (size + 1);
    if (fread (buf, 1, size, fd) != size) {
	fprintf (stderr, "read (file %s, %d bytes) failed\n", name, size);
	exit (1);
    }
    buf[size] = '\0';
    fclose (fd);

    /* find how many lines */
    cnt = 0;
    p = buf;
    while (*p != '\0') {
	if (*p == '\n')
	    cnt++;
	p++;
    }
    if (p > buf && p[-1] != '\n')
	cnt++;

    if (N_open_files >= MAX_N_FILES) {
	fprintf (stderr, "Too many open files\n");
	exit (1);
    }
    of = Open_files + N_open_files;

    of->name = MISC_malloc (strlen (name) + 1);
    strcpy (of->name, name);
    of->fd = fd;
    of->n_lines = cnt;
    of->loffs = MISC_malloc ((cnt + 1) * sizeof (int));
    of->text = buf;

    cnt = 0;
    p = buf;
    of->loffs[cnt] = p - buf;
    while (*p != '\0') {
	if (*p == '\n') {
	    cnt++;
	    of->loffs[cnt] = p - buf + 1;
	}
	p++;
    }
    of->loffs[of->n_lines] = p - buf;
    N_open_files++;

    if (sizep != NULL)
	*sizep = size + 1;
    if (nlinesp != NULL)
	*nlinesp = of->n_lines;
    return (N_open_files - 1);
}

/******************************************************************

    Returns the line of number "line" of open file "fd" in buf of size
    "b_size". Returns the number of bytes of the line or -1 on failure.

******************************************************************/

int FF_get_line (int fd, int line, char *buf, int b_size) {
    int len;

    if (fd < 0 || fd >= N_open_files) {
	fprintf (stderr, "Bad open file fd %d\n", fd);
	exit (1);
    }
    if (line < 0 || line >= Open_files[fd].n_lines)
	return (-1);
    len = Open_files[fd].loffs[line + 1] - Open_files[fd].loffs[line];
    if (len >= b_size) {
	fprintf (stderr, "Buffer too small for a line (%d), file %s\n", 
					line, Open_files[fd].name);
	exit (1);
    }
    strncpy (buf, Open_files[fd].text + Open_files[fd].loffs[line], len);
    buf[len] = '\0';
    return (len);
}


/******************************************************************

    Writes "len" bytes of "text" to file "name".

******************************************************************/

void FF_output_text (char *name, char *text, int len) {
    typedef struct {
	char *name;
	int fd;
    } Out_file_t;
    static Out_file_t out_files[MAX_N_FILES];
    static int n_out_files = 0;
    int i;

    for (i = 0; i < n_out_files; i++) {
	if (strcmp (out_files[i].name, name) == 0)
	    break;
    }
    if (i >= n_out_files) {	/* we must open the file */
	if (n_out_files >= MAX_N_FILES) {
	    fprintf (stderr, "Too many open output files\n");
	    exit (1);
	}
	out_files[n_out_files].name = MISC_malloc (strlen (name) + 1);
	strcpy (out_files[n_out_files].name, name);
	out_files[n_out_files].fd = open (name, 
				O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (out_files[n_out_files].fd < 0) {
	    fprintf (stderr, "Creating output file %s failed\n", name);
	    exit (1);
	}
	n_out_files++;
    }

    if (write (out_files[i].fd, text, len) != len) {
	fprintf (stderr, "Writing %d bytes to file %s failed\n", len, name);
	exit (1);
    }
}

/******************************************************************

    Writes a file header to file "out_file".

******************************************************************/

void FF_outout_file_header (char *out_file, char *text) {
    static char *rcs = "\n\
/*\n\
 * RCS info\n\
 * $Author: jing $\n\
 * $Locker:  $\n\
 * $Date: 2010/10/26 21:04:26 $\n\
 * $Id: ftc_file.c,v 1.1 2010/10/26 21:04:26 jing Exp $\n\
 * $Revision: 1.1 $\n\
 * $State: Exp $\n\
 */\n\n";
    static char *stars = "********************************************************************";

    FF_output_text (out_file, "\n/", 2);
    FF_output_text (out_file, stars, strlen (stars));
    FF_output_text (out_file, "\n\n", 2);
    FF_output_text (out_file, text, strlen (text));
    FF_output_text (out_file, "\n", 1);
    FF_output_text (out_file, stars, strlen (stars));
    FF_output_text (out_file, "/\n", 2);

    FF_output_text (out_file, rcs, strlen (rcs));
}


/******************************************************************

    Modifies file "name" to set "#define FTC_MAIN".

******************************************************************/

void FF_set_main (char *name) {
    char buf[1024];
    int fd, off;

    fd = open (name, O_RDWR);
    if (fd < 0) {
	fprintf (stderr, "Opening file %s for setting main failed\n", name);
	exit (1);
    }
    off = 0;
    while (1) {
	char *p;
	int ret = read (fd, buf, 1023);
	buf[ret] = 0;
	if ((p = strstr (buf, "/* #define FTC_MAIN */")) != NULL) {
	    char *nt = "#define FTC_MAIN      ";
	    if (lseek (fd, off + (p - buf), SEEK_SET) < 0 ||
		write (fd, nt, strlen (nt)) != strlen (nt)) {
		fprintf (stderr, "Updating file %s for setting main failed\n",
						name);
		exit (1);
	    }
	    break;
	}
	if (ret < 1023) {
	    fprintf (stderr, "Pattern not found in %s for setting main\n",
						name);
	    exit (1);
	}
	off += ret - 32;
	if (lseek (fd, off, SEEK_SET) < 0) {
	    fprintf (stderr, "lseek %s for setting main failed\n", name);
	    exit (1);
	}
    }
    close (fd);
}




