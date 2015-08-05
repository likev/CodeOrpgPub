
/******************************************************************

  This is a tool that verifies the data in an archive 2 file
  against an xml file.
	
******************************************************************/

/* 
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:14:48 $
 * $Id: validate_a2.c,v 1.8 2014/03/18 18:14:48 jeffs Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */  
#include <rpgcs.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <bzlib.h>
#include <zlib.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <basedata.h>
#include <generic_basedata.h> 
#include <glib.h>
#include <math.h>
#include <getopt.h>
#include "validate_a2_xml.h"

#define LOCAL_NAME_SIZE 128		/* maximum name size */
#define MESSAGE_SIZE 2432		/* NCDC message size */
#define FILE_HEADER_SIZE 24		/* NCDC file header size */
#define BITS_PER_BYTE 8
/*#define MAX_RADIAL_SIZE 65535*/

static int Verbose;			/* verbose mode flag */
static char Dir_name[LOCAL_NAME_SIZE];	/* the directory to read from */
static char Lb_name[LOCAL_NAME_SIZE];	/* output LB name */
static int Decompress = 0;		/* need decompress */

static int Cr_file_done = 0;		/* the current file is completed */
static int File_fd = -1;		/* current file descriptor */

static char Cr_file_name[LOCAL_NAME_SIZE];
					/* name of the current file */
static char xml_file_name[LOCAL_NAME_SIZE];

static int Bytes_read = 0;		/* number of bytes read from file */
static char *Buffer = NULL;		/* buffer for message segments */
static int Buf_size = 500000;		/* size of Buffer */

static int Discard_data_already_in_file = 1;
					/* data in the file is discarded when
					   the file is open - We only output
					   new data */

static int Seg_size = 0;		/* size of the current data segmant */
static char Radar_id[8] = "";		/* 4-letter radar ID read from LDM */
char* msgPrintBuf = NULL;
char* errBuf = NULL;
gchar* summaryBuf = NULL;
static int toPrint = 0;
static int tempVal = 0;
int printValues = 0;
int segment_number = 0;
static int num_type_errors = 0;
int printMsg[32] = {0};
static int numMsg[32] = {0};
static int printTypes = 0;
static int printHdrs = 0;
static int glob_vol_date = 0;
static int glob_vol_time = 0;
static char* struct_buffer = NULL;
static int struct_buffer_end = 0;
static int finishedMsg15 = 0;
static int finishedMsg13 = 0;
static int finishedMsg18 = 0;
static int finished_metadata = 0;
static int print_only_radials = 0;
static int print_only_metadata = 0;
int verbosity_level = 1;
int curr_type = 0;
static char* binary_output = NULL;
xmlDocPtr globDoc = NULL;
xmlXPathContextPtr globXpathCtx = NULL;
xmlXPathObjectPtr globXpathObj = NULL;

/* local functions */
int Julian_to_date(int julian_date, int *year, int *month, int *day);
int Milliseconds_to_time(unsigned int time, int *hour, int *minute, 
                         int *second, int *mills);
static int Read_options (int argc, char **argv);
static void Print_usage (char **argv);
static char *Get_full_path (char *name);
static void Process_current_file ();
static int Process_data (char *buffer, int n_bytes);
static void Decompress_and_process_file ();
static int check_message_sizes(int n_bytes, int size, int type);
static int check_message_type(int type);
static void reset_struct_buffer();
static int allocate_struct_buffer(char* buffer, int type, int size, int seg_num, int num_segs);
static int perform_type_two_check(int type, int size);
static void Print_Message_15(char* buffer);
static void Print_Message_31(char *buffer);
static void check_clutter_map_msg_data_ranges(char* buffer);
static int generate_temp_filename();
static int does_file_exist(char* filename);
static int is_valid_file_extension(char* filename);
static int uncompress_to_file(int f_num, char *filename);
static void print_msg_hdr();
static void print_msg_buf();
static int get_message_num_segs(int type);
static void Print_summary();


/*******************************************************************************
* Function: main
*
* Description: The main function.  This code is based upon read_ldm code.
*
* Inputs: int argc - number of arguments
*         char **argv - array of strings
*
* Returns: 0 upon failure
*          1 upon success
*
* Author: Jason Close
*
*******************************************************************************/
int main (int argc, char **argv) {
    if (Read_options (argc, argv) != 0)
	    exit (1);
    if (Buffer == NULL)
	    Buffer = MISC_malloc (Buf_size);
    summaryBuf = g_strdup_printf("\n");
    xmlXPathInit();
    xmlInitParser();
    xmlKeepBlanksDefault(0);
    initiate_xpath_vars(xml_file_name, &globDoc, &globXpathCtx);
    Process_current_file ();
    Print_summary();

    /* Clear out libxml stuff */
    xmlXPathFreeContext(globXpathCtx);
    xmlFreeDoc(globDoc);
    xmlCleanupParser();
    xmlMemoryDump();
    if (binary_output != NULL)
        free(binary_output);
    if (summaryBuf != NULL)
        g_free(summaryBuf);
    if (Decompress == 3)
        remove(Cr_file_name);
    exit (0);
} /* end main */

/*******************************************************************************
* Function: Process_current_file 
*
* Description: This function looks at the file, and based upon the file
*              extension and the flags passed, will make decisions.  If the 
*              file is compressed, it will try to decompress the file.  If the
*              file is not compressed, it will read in the file and break it 
*              apart segment by segment, message by message
*
* Inputs: none
*
* Returns: none
*
*******************************************************************************/
static void Process_current_file () {
    int ret;
    segment_number = 0;

    if (Decompress == 0)
        Decompress = is_valid_file_extension(Cr_file_name);

    /* This means we have a .Z, .bz, .gz, or .zip file */
    /*if (fExt)*/
    if (Decompress == 3)
    {
        int fNum = generate_temp_filename();
        if (fNum == uncompress_to_file(fNum, Cr_file_name))
        {
            memset(Cr_file_name, '\0', LOCAL_NAME_SIZE);
            sprintf(Cr_file_name, "/tmp/uncompress%d", fNum);
        } /* end if fNum == uncompress_to_file */
        else
            return;
    } /* end if fExt */
    if (File_fd < 0) { 			/* open the file */
        File_fd = open (Cr_file_name, O_RDONLY);
        if (File_fd < 0) 
        {
            fprintf(stdout, "open %s failed (errno %d)\n", 
            Get_full_path (Cr_file_name), errno);
            if (errno == E2BIG)
                fprintf(stdout, "The file passed in has bad syntax.\n");
            else if (errno == EACCES)
                fprintf(stdout, "A file permission occurred with %s\n", Cr_file_name);
            else if (errno == EADDRINUSE)
                fprintf(stdout, "The is already in use by another program\n");
            else if (errno == EADDRNOTAVAIL)
                fprintf(stdout, "This file is already in use by this process and there was an error.\n");
            else if (errno == ENOENT)
                fprintf(stdout, "This file does not exist.\n");
            Cr_file_done = 1;
            return;
        }
        Bytes_read = 0;
    }

    if (Bytes_read < FILE_HEADER_SIZE) {	/* read the file header */
        /* read in the first 24 bytes */
        ret = MISC_read (File_fd, 
                         (char *)Buffer + Bytes_read, FILE_HEADER_SIZE - Bytes_read);
        if (ret < 0) {
            fprintf(stdout, 
                       "read file header failed (file %s, errno %d)\n", 
            Get_full_path (Cr_file_name), errno);
            Cr_file_done = 1;
            return;
        }
        Bytes_read += ret;
    } /* end if Bytes_read < FILE_HEADER_SIZE */

    if (Bytes_read >= FILE_HEADER_SIZE) 
    {   
        /* check file header */
        NEXRAD_vol_title* vol = (NEXRAD_vol_title*)(Buffer);
        if (strncmp(vol->filename, "ARCHIVE2.", 9) != 0 && 
            strncmp(vol->filename, "AR2", 3) != 0) {
            fprintf(stdout, "file header check failed (file %s). Gave a filename of %s\n", Get_full_path (Cr_file_name), vol->filename);
            Cr_file_done = 1;
            return;
        } /* end if strncmp ... */
        
        MISC_swap_longs(2, &vol->julian_date);
        glob_vol_date = vol->julian_date;
        glob_vol_time = vol->millisecs_past_midnight;
        strncpy(Radar_id, (char *)Buffer + 20, 4);
        Radar_id[4] = '\0';
        Bytes_read = 0;
        Seg_size = 0;
    }
    else
        return;

    if ((Decompress == 1) || (Decompress == 2)) {
        Decompress_and_process_file ();
        return;
    }

    /* The following section is from Eddie for supporting uncompr Message 31 */
    while (1) {
	short size, type, *spt;
	unsigned char *cpt;
	int bytes_processed;
	int read_size = 16;  /* Read first 16 bytes of the header */
	type = 1;
	while (Bytes_read < read_size) {
	    if (read_size + 1024 > Buf_size) 
            {
		if (Buffer != NULL)
		    free (Buffer);
		Buf_size = read_size + 1024;
		Buffer = MISC_malloc (Buf_size);
	    } /* end if (read_size + 1024 > Buf_size) */
	    ret = MISC_read (File_fd, (char *)Buffer + Bytes_read,
			         		read_size - Bytes_read);
            /* End of file */
            if (ret == 0)
                return;

	    if (ret < 0) {
		fprintf(stdout, "read data failed (file %s, errno %d)\n",
                           Get_full_path (Cr_file_name), errno);
		Cr_file_done = 1;
		return;
	    }
	    spt = (short *)Buffer;
	    cpt = (unsigned char *)Buffer;
	    size = spt[6];
#ifdef LITTLE_ENDIAN_MACHINE
	    size = SHORT_BSWAP (size);
#endif
	    type = cpt[15];
	    size *= 2;
	    if (type == 31)
		read_size = size + 12;
	    else
		read_size = MESSAGE_SIZE;
	    Bytes_read += ret;
	} /* end while Bytes_read < read_size */
	
        if ((type != 31) && (Bytes_read < MESSAGE_SIZE)) 
        {
            /* patial message read */
	    Discard_data_already_in_file = 0;
	    return;
	} /* if ((type != 31) && (Bytes_read < MESSAGE_SIZE)) */
   	
        bytes_processed = Process_data ((char *)Buffer, Bytes_read);

        if (num_type_errors > 5)
        {
            print_msg_buf();
            print_error_buf();
            clear_error_buf();
            return;
        } /* end if num_type_errors > 5 */

	if (bytes_processed < Bytes_read) 
        {
	    off_t off;
	    off = lseek (File_fd, bytes_processed - Bytes_read, SEEK_CUR);
	    if (off == ((off_t)-1))
		fprintf (stderr, 
			"Error %d seeking %d bytes from current position\n",
			errno, bytes_processed - Bytes_read);
        } /* end if bytes_processed < Bytes_read */

        /*print_msg_buf();*/
        print_msg_hdr();
        print_error_buf();
        clear_error_buf();
      	Bytes_read = 0;
        segment_number++;
    } /* end while(1) */
} /* End Process_current_file */

/*******************************************************************************
* Function: Decompress_and_process_file 
*
* Description: This function reads in the compressed file, allocates a properly
*              sized buffer, then decompresses the file piece by piece.  After
*              each piece is decompressed, it is processed, and then the 
*              error or print messages are printed.
*
* Inputs: none
*
* Returns: none
*
*******************************************************************************/
static void Decompress_and_process_file () {
    int ret;
    int tmp;
    unsigned long dest_len_tmp;
    tmp = -1;
    while (1) {
	if (Seg_size <= 0) { 			/* read the segment size */
	    if (Bytes_read < 4) {
		ret = MISC_read (File_fd, 
				(char *)Buffer + Bytes_read, 4 - Bytes_read);
		if (ret < 0) {
		    fprintf(stdout, 
			    "read segment size failed (file %s, errno %d)\n", 
			    Get_full_path (Cr_file_name), errno);
		    Cr_file_done = 1;
		    return;
		} /* end if (ret < 0) */
		Bytes_read += ret;
	    } /* end if (Bytes_read < 4) */
	    
            if (Bytes_read >= 4) {
		Seg_size = *((int *)((char *)Buffer));
#ifdef LITTLE_ENDIAN_MACHINE
		Seg_size = INT_BSWAP (Seg_size);
#endif
		Bytes_read = 0;
		if (Seg_size < 0)
		    Seg_size = -Seg_size;
		if (Seg_size <= 0 || Seg_size >= 10000000) {
		    fprintf(stdout, "Bad segment size (file %s, size %d)\n",
				Get_full_path (Cr_file_name), Seg_size);
		    Cr_file_done = 1;
		    return;
		} /* end if Seg_size <= 0 ... */
	    } /* end if (Bytes_read >= 4) */
	    else {
		Discard_data_already_in_file = 0;
		return;
	    } /* end else */
	} /* end if Seg_size <= 0 */

        /* if we have a size */
	if (Seg_size > 0) {
	    static char *dest = NULL;
	    static int dest_bsize = 500000;
	    int dest_len, bytes_processed;

	    if (Bytes_read < Seg_size) {
                /* At this point, Buf_size is 500,000 */
		if (Seg_size + 10000 > Buf_size) {
		    if (Buffer != NULL)
			free (Buffer);
                    /* Now it is the segment size plus 10,000 */
		    Buf_size = Seg_size + 10000;
                    /* Allocate Buffer */
		    Buffer = MISC_malloc (Buf_size);
		} /* end if (Seg_size + 10000 > Buf_size) */

                /* Bytes_read should put us at the first byte past the 4-byte
                 * size segment */
		ret = MISC_read (File_fd, (char *)Buffer + Bytes_read, 
						Seg_size - Bytes_read);
		if (ret < 0) {
		    fprintf(stdout, 
			    "read data failed (file %s, errno %d)\n", 
			    Get_full_path (Cr_file_name), errno);
		    Cr_file_done = 1;
		    return;
		}
		Bytes_read += ret;
	    } /* end if (Bytes_read < Seg_size) */

	    if (Bytes_read < Seg_size) {
		Discard_data_already_in_file = 0;
		return;
	    }

            /* So this is essentially a while loop that keeps reallocating
             * memory until we have enough buffer space to decompress the 
             * bzip2'd message. We start at 500,000, and then double each time.
             * */
	    while (1) {
		if (dest == NULL)
                    /* dest_bsize is 500,000 */
		    dest = MISC_malloc (dest_bsize);
		dest_len = dest_bsize;
                if (Decompress == 1)
                {
                    ret = BZ2_bzBuffToBuffDecompress (dest, 
                    (unsigned int *)&dest_len, (char *)Buffer, Seg_size, 0, 0);
                    if (ret == BZ_OUTBUFF_FULL) {
                        if (dest != NULL)
                            free (dest);
                        dest_bsize *= 2;                   
                        dest = MISC_malloc (dest_bsize);
                        continue;
                    } /* end if */
                    if (ret < 0) {
                        fprintf(stdout, 
                                "BZ2 decompress failed (file %s), returns %d\n", 
                                Get_full_path (Cr_file_name), ret);
                        Cr_file_done = 1;
                        return;
                    } /* end if ret < 0 */
                } /* end if Decompress == 1 */
                else if (Decompress == 2)
                {
                    dest_len_tmp = dest_len;
                    ret = uncompress((unsigned char*)dest, &dest_len_tmp, 
                            (unsigned char *)Buffer, Seg_size);
                    dest_len = dest_len_tmp;
                    if (ret == Z_MEM_ERROR) {
                        if (dest != NULL)
                            free(dest);
                        dest_bsize *= 2;
                        dest = MISC_malloc(dest_bsize);
                        continue;
                    } /* end ret == Z_MEM_ERROR */
                    if (ret < 0) {
                        fprintf(stdout, 
                                "GZ decompress failed (file %s), returns %d\n", 
                                Get_full_path(Cr_file_name), ret);
                        Cr_file_done = 1;
                        return;
                    } /* end ret < 0 */
                } /* end else if Decompress == 2 */
		    break;
	    } /* end while */
        /* if successful, dest_len is set to the size of  
         * the uncompressed data and BZ_OK is returned */
	    Bytes_read = dest_len;
        toPrint = 1;
	    bytes_processed = 0;

            /* This is where the segments get processed */
	    while (Bytes_read > bytes_processed) {
                tempVal = bytes_processed;
		bytes_processed += 
			Process_data ((char *)dest + bytes_processed, 
					Bytes_read - bytes_processed);

                if (num_type_errors > 5)
                {
                    print_msg_buf();
                    print_error_buf();
                    clear_error_buf();
                } /* end if num_type_errors > 5 */
	    } /* end while Bytes_read */
            /* Print all of the errors/warnings */
            print_msg_buf();
            print_error_buf();
            clear_error_buf();
            Bytes_read = 0;
            Seg_size = 0;
            free(dest);
            dest = NULL;
        } /* end if (Seg_size > 0) */
    } /* end while(1) */
} /* end Decompress_and_process_file */

/*******************************************************************************
* Function: Process_data 
*
* Description: Processes a complete NCDC message in "buffer".
*
* Inputs: char *buffer - The buffer holding the raw data
*         int n_bytes - The size of buffer in bytes
*
* Returns: int - The message size that was read.
*
*******************************************************************************/
static int Process_data (char *buffer, int n_bytes) 
{
    short size, type, tmp;
    segment_number++;
    RDA_RPG_message_header_t* msg_header = (RDA_RPG_message_header_t*)(buffer + 12);
    xpath_specific_object(buffer + 12, "RDA_RPG_message_header_t", 0, &globDoc, &globXpathCtx, &globXpathObj);
    size = msg_header->size;
    size *= 2;
    type = msg_header->type;
    
    if (printTypes)
        fprintf(stdout, "Segment %d: message type %d\n", segment_number, type);
    /* unused message - discarded */
    if (type == 0)			
        return (MESSAGE_SIZE);

    /* Anything under 0 or over 50 is discarded */
    if ((tmp = check_message_type(type)))
        return tmp;

    /* Figure out what to do if it is a message of type 2 */
    if ((tmp = perform_type_two_check(type, size)))
        return tmp;

    /* Don't care about any other messages than the ones the user requested */
    if (!printMsg[type])
    {
        if (type == 31)
            return size + 12;
        else
            return (MESSAGE_SIZE);
    }

    numMsg[type]++;
    if ((tmp = check_message_sizes(n_bytes, size, type)))
        return tmp;

    /* This is bad.  There is something wrong here. */
    if (msg_header->seg_num > msg_header->num_segs)
    {
        if (printMsg[0])
            fprintf(stdout, "ERROR: Segment: %d (type %d): In the header, the segment number(%d) is greater than the number of segments(%d)\n", segment_number, type, msg_header->seg_num, msg_header->num_segs);
        return (MESSAGE_SIZE);
    }

    /* Set our global variable */
    curr_type = type;
    /* Here is where we are allocating some space for the messages that 
     * are made up of multiple segments (excluding Type 31) */
    if ((msg_header->num_segs > 1) && ((type == 15) || (type == 13) || (type == 18)))
    {
        if ((tmp = allocate_struct_buffer(buffer, type, size, msg_header->seg_num, msg_header->num_segs)))
            return tmp;
    } /* end if ((msg_header->num_segs > 1) && ... */


    if ((type >= 1) && (type <= 12))
    {
        initiate_xpath(buffer + 12 + sizeof(RDA_RPG_message_header_t), 
            type, &globDoc, &globXpathCtx, &globXpathObj);
        return (MESSAGE_SIZE);
    }

    if (((type == 13) && (!finishedMsg13)) || ((type == 18) && (!finishedMsg18))) {
       if (msg_header->num_segs != msg_header->seg_num)
           return (MESSAGE_SIZE);
       initiate_xpath(struct_buffer, type, &globDoc, &globXpathCtx, &globXpathObj);
       reset_struct_buffer();
       if (type == 13)
          finishedMsg13 = 1;
       else
          finishedMsg18 = 1;
       return (MESSAGE_SIZE);
    }

    if ((type == 15) && (!finishedMsg15)) {
       if (msg_header->num_segs != msg_header->seg_num)
          return (MESSAGE_SIZE);
       Print_Message_15(struct_buffer);
       reset_struct_buffer();
       finishedMsg15 = 1;
       return (MESSAGE_SIZE);
    }

    /* Message 31 is the only message that is variable length */
    if (type == 31) {
        Print_Message_31(buffer + 12);
    	return (size + 12);
    } 
    return (MESSAGE_SIZE);
} /* end Process_data */

static int check_message_type(int type)
{
    if (type < 0 || type > 50)	{	/* not a NCDC message */
        if (printMsg[0])
        {
            gchar* tmpMsg = NULL;
            g_strdup_printf("ERROR: Segment %d - Unexpected data type (%d)\n", 
                    segment_number, type);
            add_msg_to_error_buf(tmpMsg);
            g_free(tmpMsg);
        }
        num_type_errors++;
        return (MESSAGE_SIZE);
    }
    return 0;
}
static int check_message_sizes(int n_bytes, int size, int type)
{
    /* Everything should be MESSAGE_SIZE.  Throw an error */
    if ((type != 31) && (size > MESSAGE_SIZE)) 
    {
        if (printMsg[0])
            fprintf(stdout,
                "ERROR: Segment: %d (type %d) - Message size larger than expected - %d instead of (%d).  Discarding message.\n", 
                segment_number, type, size, MESSAGE_SIZE);
        if (type == 13)
           finishedMsg13 = 2;
        if (type == 15)
           finishedMsg15 = 2;
        if (type == 18)
           finishedMsg18 = 2;
        return (MESSAGE_SIZE);
    } /* end if type != 31 */
    
    if ((n_bytes < size) || ((type != 31) && (n_bytes < MESSAGE_SIZE))) {
        fprintf(stdout, " ERROR: - Read problem - Data bytes (%d) less than expected (%d)\n", 
                    n_bytes, size);
        if (type == 13)
           finishedMsg13 = 2;
        if (type == 15)
           finishedMsg15 = 2;
        if (type == 18)
           finishedMsg18 = 2;
        if (type == 31)
            return size;
        return (MESSAGE_SIZE);
    } /* end if */
    return 0;
}

static void reset_struct_buffer()
{
    if (struct_buffer != NULL)
        free(struct_buffer);
    struct_buffer = NULL;
    struct_buffer_end = 0;
}

static int allocate_struct_buffer(char* buffer, int type, int size, int seg_num, int num_segs)
{
    /* finishedMsg15 will be 1 when the type is 1, and we hit a 
     * packet where the num_segs = seg_num */
    if ((type == 15) && (finishedMsg15))
    {
        if (finishedMsg15 == 1)
            fprintf(stdout, "ERROR: There were extra segments for message 15.  Received a segment of type 15 at segment number %d\n", seg_num);
        finishedMsg15 = 2;
        return (MESSAGE_SIZE);
    } /* end type == 15 */

    if ((type == 13) && (finishedMsg13))
    {
        if (finishedMsg13 == 1)
            fprintf(stdout, "ERROR: There were extra segments for message 13.  Received a segment of type 13 at segment number %d\n", seg_num);
        finishedMsg13 = 2;
        return (MESSAGE_SIZE);
    } /* end type == 13 */

    if ((type == 18) && (finishedMsg18))
    {
        if (finishedMsg18 == 1)
            fprintf(stdout, "ERROR: There were extra segments for message 18.  Received a segment of type 18 at segment number %d\n", seg_num);
        finishedMsg18 = 2;
        return (MESSAGE_SIZE);
    } /* end type == 18 */

    if (seg_num == 1)
    {
        if (struct_buffer != NULL)
        {
            fprintf(stdout, "Resetting struct buffer for type %d\n", type);
            reset_struct_buffer();
        }
        struct_buffer = MISC_malloc(get_message_num_segs(type) * (MESSAGE_SIZE - sizeof(RDA_RPG_message_header_t)));
        memset(struct_buffer, '\0', MESSAGE_SIZE - sizeof(RDA_RPG_message_header_t));
    } /* end msg_header->seg_num == 1 */

    memcpy(struct_buffer + struct_buffer_end, 
           buffer + 12 + sizeof(RDA_RPG_message_header_t), 
           MESSAGE_SIZE - sizeof(RDA_RPG_message_header_t) - 12);
    struct_buffer_end = struct_buffer_end + MESSAGE_SIZE - sizeof(RDA_RPG_message_header_t) - 16;
    return 0;
}

static int perform_type_two_check(int type, int size)
{
    /* Message 2 is the last message in the metadata.  Message 2 is the only
     * message type that is in both the metadata and the radial data*/
    if (type == 2)
    {
        /* At this point, we are encountering the first message 2 segment, 
         * which means we are at the last metadata segment.  If we are 
         * only printing radials, then we want to print messages of type 2
         * in the radial data, but not the metadata. */
        if ((!finished_metadata) && print_only_radials)
        {
            finished_metadata = 1;
            return (MESSAGE_SIZE);
        }
        /* if this is true, then we past the metadata, but we have found
         * a message 2 in the radial data.  We dont want to print that here*/
        if (finished_metadata && print_only_metadata)
          return (MESSAGE_SIZE);

        if (size != 96) 
        {
    	    fprintf(stdout, "ERROR: unexpected RDA status size (%d)\n", size);
	        return (MESSAGE_SIZE);
    	} /* end if size != 96 */

        finished_metadata = 1;
    }
    return 0;
}
static void Print_Message_15(char* buffer)
{
    ORDA_clutter_map_t* msg = (ORDA_clutter_map_t *)buffer;
    MISC_swap_shorts(struct_buffer_end / 2, (short *) &msg->date);
    check_clutter_map_msg_data_ranges(buffer);
}

/*******************************************************************************
* Function: Print_Message_31
*
* Description: Performs the byte swaps for a message type of 31, then 
*              calls check_basedata_message_data_range to check the 
*              values.  It does this after it determines what "type" of 
*              Message 31 it is.  
*
* Inputs: char *buffer - The buffer holding the raw data
*
* Returns: none
*
*******************************************************************************/
static void Print_Message_31(char *buffer)
{
   Generic_basedata_t* rec = (Generic_basedata_t *) buffer;
   int no_of_datum, i;
   int to_external;
   to_external = 0;

   /* This will byte swap all of the Generic_basedata_header_t data */
   xpath_specific_object(buffer + sizeof(RDA_RPG_message_header_t), "Generic_basedata_t", 31, 
           &globDoc, &globXpathCtx, &globXpathObj);

   no_of_datum = rec->base.no_of_datum;

   /* Check if the data is compressed.  If so decompress and 
      set the compression flag to 0. M31_BZIP2_COMP = 1 M31_ZLIP_COMP = 2 */
   if( (rec->base.compress_type == M31_BZIP2_COMP) || (rec->base.compress_type == M31_ZLIB_COMP) ){
       int ret, method, src_len, offset;
       char *src;
       static char *dest = NULL;
       static int dest_len = 0;

       /* malloc a temporary buffer to hold the decompressed data. */
       if( (rec->base.radial_length > dest_len) || (dest == NULL) ){
           dest = realloc( dest, rec->base.radial_length );
           if( dest == NULL ){
              fprintf(stderr, " * Problem allocating memory.\n");
              return;
           }
       }

       /* Set the decompressed size. */
       dest_len = rec->base.radial_length;

       /* Determine which compression method was used. */
       if( rec->base.compress_type == MISC_BZIP2 )
           method = MISC_BZIP2;
       else
           method = MISC_GZIP;

       /* Decompress the data. */
       offset = sizeof(Generic_basedata_t) + no_of_datum*sizeof(int);
       src = buffer + offset;
       src_len = rec->msg_hdr.size*sizeof(short) - offset;

       /* Performs a bz2bufftobuffdecompress */
       ret = MISC_decompress( method, src, src_len, dest, dest_len );
       if ( ret < 0 ) {
           fprintf(stderr, "MISC_decompress Failed\n");
           return;
       }

       /* Set the compress_type flag in header to NO COMPRESSION, 
          set the size field in the radial header based on the 
          decompressed size, then copy the decompressed data
          back to the original source buffer. */
       rec->base.compress_type = 0;
       rec->msg_hdr.size = (dest_len + 1)/sizeof(short);
       memcpy( src, dest, dest_len );
   }

   for( i = 0; i < no_of_datum; i++ )
   {
      Generic_any_t *data_block;
      char type[5];
      int offset;
      offset = 0;
      if (to_external)
          offset = rec->base.data[i];
#ifdef LITTLE_ENDIAN_MACHINE
      MISC_swap_longs( 1, (long *) &(rec->base.data[i]) );
#endif
      if (!to_external)
          offset = rec->base.data[i];

      data_block = (Generic_any_t *)
                 (buffer + sizeof(RDA_RPG_message_header_t) + offset);

      /* Convert the name to a string so we can do string compares. */
      memset( type, 0, 5 );
      memcpy( type, data_block->name, 4 );

      if( type[0] == 'R' )
      {
         if( strcmp( type, "RRAD" ) == 0 )
            xpath_specific_object((char*)data_block, "Generic_rad_t", 31, 
                    &globDoc, &globXpathCtx, &globXpathObj);
         else if( strcmp( type, "RELV" ) == 0 )
            xpath_specific_object((char*)data_block, "Generic_elev_t", 31, 
                    &globDoc, &globXpathCtx, &globXpathObj);
         else if( strcmp( type, "RVOL" ) == 0 )
            xpath_specific_object((char*)data_block, "Generic_vol_t", 31, 
                    &globDoc, &globXpathCtx, &globXpathObj);
         else
            fprintf( stderr, "Undefined/Unkwown Block Type.\n" );
      }
      else if( type[0] == 'D' )
      {
          if(strcmp( type, "DREF" ) == 0 )
              xpath_specific_object((char*)data_block, "Generic_moment_t_ref", 31, 
                      &globDoc, &globXpathCtx, &globXpathObj);
          else if(strcmp( type, "DVEL" ) == 0 )
              xpath_specific_object((char*)data_block, "Generic_moment_t_vel", 31, 
                      &globDoc, &globXpathCtx, &globXpathObj);
          else if(strcmp( type, "DSW " ) == 0 )
              xpath_specific_object((char*)data_block, "Generic_moment_t_sw", 31, 
                      &globDoc, &globXpathCtx, &globXpathObj);
          else if(strcmp( type, "DZDR" ) == 0 )
              xpath_specific_object((char*)data_block, "Generic_moment_t_zdr", 31, 
                      &globDoc, &globXpathCtx, &globXpathObj);
          else if(strcmp( type, "DPHI" ) == 0 )
              xpath_specific_object((char*)data_block, "Generic_moment_t_phi", 31, 
                      &globDoc, &globXpathCtx, &globXpathObj);
          else if(strcmp( type, "DRHO" ) == 0 )
              xpath_specific_object((char*)data_block, "Generic_moment_t_rho", 31, 
                      &globDoc, &globXpathCtx, &globXpathObj);
          else
              xpath_specific_object((char*)data_block, "Generic_moment_t", 31, 
                      &globDoc, &globXpathCtx, &globXpathObj);
      } /* end if (type[0] == 'D') */
      else {
         fprintf( stderr, "Invalid Data Block Type: %c\n", type[0] );
         return;
      } /* end else */
   } /* end for loop */
} /* end Print_Message_31 */

/*******************************************************************************
 * Function: check_clutter_map_msg_data_ranges
 * 
 * Description: This function does all of the value checking for the messages 
 *              of type 15 (Clutter Map Filter). The values should be 
 *              in line with those specified in the ICD.  If a value is not 
 *              in line, a message will be added to errBuf.  If the printValues
 *              value is set, then a message with each value will be printed.
 * 
 * Inputs: char* - The buffer pointer.
 * 
 * Returns: nothing
 *
 ******************************************************************************/
static void check_clutter_map_msg_data_ranges(char* buffer)
{
    ORDA_clutter_map_t* msg = (ORDA_clutter_map_t*)buffer;
    short * valBuf;
    valBuf = (short *)buffer;

    check_unsigned_short_range_values(msg->date, 1, 65535, 15, "date", 0, 1);
    check_short_range_values(msg->time, 0, 1440, 15, "time", 0, 1);
    check_unsigned_short_range_values(msg->num_elevation_segs, 1, 5, 15, "num_elevation_segs", 0, 1);
    if (msg->num_elevation_segs > MAX_ELEVATION_SEGS_ORDA)
    {
        gchar* tmp = NULL;
        tmp = g_strdup_printf(" - Segment %d - Will not investigate message type 15 any further because the number of elevation segments is more than allowed (%d > %d)\n", segment_number, msg->num_elevation_segs, MAX_ELEVATION_SEGS_ORDA);
        return;
    }

    /**
     * We can't just typecast the rest to an ORDA_clutter_map because the data
     * is in a different format.  The ORDA_clutter_map_t struct is a fixed 
     * size, whereas the data is not. So we just need to look at the offsets
     * and go from there.
     **/
    int i, j, k, offset, num_zones;
    offset = 3;
    for (i = 0; i < msg->num_elevation_segs; i++)
    {
        if ((printValues) && (printMsg[15]) && (verbosity_level >= 3))
            fprintf(stdout, "Segment %d: Starting elevation segment %d\n", segment_number, i);
        for (k = 0; k < NUM_AZIMUTH_SEGS_ORDA; k++)
        {
            check_unsigned_short_range_values(valBuf[offset], 0, 25, 15, "num_zones", 0, 2);
            if ((valBuf >= 0) && (valBuf[offset] < MAX_RANGE_ZONES_ORDA))
            {
                num_zones = valBuf[offset];
                for (j = 0; j < num_zones; j++)
                {
                    offset++;
                    check_unsigned_short_range_values(valBuf[offset], 0, 2, 15, "op_code", 0, 3);
                    offset++;
                    check_unsigned_short_range_values(valBuf[offset],
                                                0, 511, 15, "range", 0, 3);
                } /* end for j */
                offset++;
             } /* end if */
         } /* end for k */
    }
} /* end function */

/*******************************************************************************
* Function: Get_full_path 
*
* Description: Returns the full path of a file
*
* Inputs: char* name - The name of the file, or the path
*
* Returns: char* buf - The full path
*
*******************************************************************************/
static char* Get_full_path (char *name) {
    static char fname[LOCAL_NAME_SIZE * 2 + 4];
    sprintf (fname, "%s/%s", Dir_name, name);
    return (fname);
}

/*******************************************************************************
* Function: Read_options 
*
* Description: Reads the input from the command line and properly sets all 
*              global flags and values
*
* Inputs: int argc - The number of arguments in argv
*         char** argv - An array of strings that consist of the command
*
* Returns: int - 1 on success, 0 on failure
*
*******************************************************************************/
static int Read_options (int argc, char **argv) 
{
    extern char *optarg;    /* used by getopt */
    extern int optind;
    int c, i;                  /* used by getopt */
    static int help = 0;
    static int all = 0;
    for (i = 0; i < argc; i++)
        if (strcmp(argv[i], "--help") == 0)
        {
            Print_usage (argv);
            return 1;
        }

    static struct option long_options[] = 
    {
        {"bzip", no_argument, &Decompress, 1}, 
        {"gzip", no_argument, &Decompress, 2}, 
        {"uncompress", no_argument, &Decompress, 3},
        {"help", no_argument, &help, 1}, 
        {"all", no_argument, &all, 1}, 
        {"Msg1", no_argument, 0, 'a'}, 
        {"Msg2", no_argument, 0, 'b'}, 
        {"Msg3", no_argument, 0, 'c'}, 
        {"Msg4", no_argument, 0, 'd'}, 
        {"Msg5", no_argument, 0, 'e'}, 
        {"Msg6", no_argument, 0, 'f'}, 
        {"Msg7", no_argument, 0, 'g'}, 
        {"Msg8", no_argument, 0, 'i'}, 
        {"Msg9", no_argument, 0, 'j'}, 
        {"Msg10", no_argument, 0, 'k'}, 
        {"Msg12", no_argument, 0, 'l'}, 
        {"Msg13", no_argument, 0, 'm'}, 
        {"Msg15", no_argument, 0, 'n'}, 
        {"Msg18", no_argument, 0, 'o'}, 
        {"Msg31", no_argument, 0, 'r'}, 
        {"PrintVals", no_argument, 0, 'p'}, 
        {"PrintHdrs", no_argument, 0, 'q'}, 
        {"verbose", required_argument, 0, 'v'}, 
        {"meta", no_argument, 0, 'M'}, 
        {"radial", no_argument, 0, 'R'}, 
        {"types", no_argument, 0, 'T'}, 
        {0, 0, 0, 0}
    };
    
    int option_index = 0;

    sprintf (Dir_name, ".");
    Lb_name[0] = '\0';
    Verbose = 1;
    while ((c = getopt_long (argc, argv, "abcdefghijklMmnopwqvzrR", long_options, &option_index)) != EOF) 
    {
        switch (c) {
            case '\0':
                break;
            case 'a':
                printMsg[1] = 1;
                break;
            case 'b':
                printMsg[2] = 1;               
                break;
            case 'c':
                printMsg[3] = 1;               
                break;
            case 'd':
                printMsg[4] = 1;               
                break;
            case 'e':
                printMsg[5] = 1;               
                break;
            case 'f':
                printMsg[6] = 1;               
                break;
            case 'g':
                printMsg[7] = 1;               
                break;
            case 'h':
		        Print_usage (argv);
                return 1;
            case 'i':
                printMsg[8] = 1;               
                break;
            case 'j':
                printMsg[9] = 1;               
                break;
            case 'k':
                printMsg[10] = 1;               
                break;
            case 'l':
                printMsg[12] = 1;               
                break;
            case 'm':
                printMsg[13] = 1;               
                break;
            case 'M':
                print_only_metadata = 1;
                printMsg[13] = 1;               
                printMsg[15] = 1;
                printMsg[18] = 1;
                printMsg[2] = 1;               
                printMsg[3] = 1;               
                printMsg[5] = 1;               
                break;
            case 'n':
                printMsg[15] = 1;
                break;
            case 'o':
                printMsg[18] = 1;
                break;
            case 'p':
                printValues = 1;
                break;
            case 'q':
                printHdrs = 1;               
                printMsg[0] = 1;
                break;
            case 'r':
                printMsg[31] = 1;
                break;
            case 'R':
                printMsg[31] = 1;
                printMsg[2] = 1;
                print_only_radials = 1;
                break;
            case 'T':
                printTypes = 1;
                break;
            case 'v':
                verbosity_level = atoi(optarg);
                break;
            case 'w':
                for (i = 0; i < 32; i++)
                    printMsg[i] = 1;
                break;
            case 'z':
                for (i = 0; i < 32; i++)
                    printMsg[i] = 1;
                printHdrs = 1;
                printValues = 1;
                break;
        } /* end switch */
    } /* end while */
    if (optind == argc)
    {
        fprintf(stdout, "ERROR: You did not enter in a path to the Archive II file.\n");
        return 1;
    }

    if (help)
    {
        Print_usage (argv);
        return 1;
    }
    
    strncpy(xml_file_name, argv[argc - 2], LOCAL_NAME_SIZE);
    xml_file_name[LOCAL_NAME_SIZE - 1] = '\0';
    strncpy (Cr_file_name, argv[argc - 1], LOCAL_NAME_SIZE);
    Cr_file_name[LOCAL_NAME_SIZE - 1] = '\0';
    return (0);
} /* end Read_options */

/*******************************************************************************
* Function: Print_usage
*
* Description: Prints the help information
*
* Inputs: char** argv - The string from the command line
*
* Returns: none
*
*******************************************************************************/
static void Print_usage (char **argv) 
{
    printf ("Usage: %s (options) xml_file file_name\n", argv[0]);
    printf ("       Options:\n");
    printf ("       -a (Print Digital Radar Data (Message Type 1))\n");
    printf ("       -b (Print RDA Status Data Data (Message Type 2))\n");
    printf ("       -c (Print Performance/Maintenance Data (Message Type 3))\n");
    printf ("       -d (Print Console Message Data (Message Type 4))\n");
    printf ("       -e (Print Volume Coverage Pattern Data (Message Type 5))\n");
    printf ("       -f (Print RDA Control Commands (Message Type 6))\n");
    printf ("       -g (Print Volume Coverage Pattern Data (Message Type 7))\n");
    printf ("       -h (Print usage and help information)\n");
    printf ("       -i (Print Clutter Censor Zones (Message Type 8))\n");
    printf ("       -j (Print Request For Data (Message Type 9))\n");
    printf ("       -k (Print Console Message Data (Message Type 10))\n");
    printf ("       -l (Print Loopback Test Data (Message Type 12))\n");
    printf ("       -m (Print Clutter Filter Bypass Map Data (Message Type 13))\n");
    printf ("       -n (Print Clutter Map Filter Data (Message Type 15))\n");
    printf ("       -o (Print RDA Adaptation Data (Message Type 18))\n");
    printf ("       -r (Print Digital Radar Data Generic Format Blocks (Message Type 31))\n");
    printf ("       -q (Print Print Message Header Data)\n");
    printf ("       -p (Print the values of the message, not just the warnings.\n");
    printf ("       -w (Check the ranges of all values and output warning messages)\n");
    printf ("       -z (Print all messages)\n");
    printf ("       --types Prints a list of each segment and its message type\n");
    printf ("       --meta Prints all metadata warnings (equivalent to passing -bcemno flags)\n");
    printf ("       --radial Prints warning messages for the radial data (Message 31 and 2)\n");
    printf ("       --gzip Use the gzip (Lempel-Ziv) decompression algorithm\n");
    printf ("       --bzip Use the bzip (Burrows-Wheeler) decompression algorithm\n");
    printf ("       --uncompress Use the Lempel-Ziv \"uncompress\" algorithm\n");
    printf ("       --verbose [1-3] Sets the verbosity level\n");
    printf ("\n");
    printf ("       Examples:\n");
    printf ("       validate_a2 -abcd archive_2.xml /my/path/to/file\n");
    printf ("            Prints the warnings for message types 1, 2, 3, and 4\n\n");
    printf ("       validate_a2 -abcdp --bzip archive_2.xml /my/path/to/file\n");
    printf ("            Prints the warnings and values for message types 1, 2, 3, and 4,\n");
    printf ("               as well as uses bzip to uncompress the file.\n\n");
    printf ("       validate_a2 -z archive_2.xml /my/path/to/file.Z\n");
    printf ("            Prints all warnings and all values for all message types,\n");
    printf ("               all after using uncompress to decompress the file.\n");
    printf ("               validate_a2 -z --uncompress /my/path/to/file.Z would have\n");
    printf ("               done the same thing.\n");
    printf ("       validate_a2 -p --meta --bzip archive_2.xml /my/path/to/file.raw\n");
    printf ("            Prints all values for the metadata in a raw file.\n");
    printf ("       validate_a2 --radial --bzip archive_2.xml /my/path/to/file.raw\n");
    printf ("            Prints all warnings for the radial data in a raw file.\n");
} /* end Print_usage */

/*******************************************************************************
* Function: Julian_to_date
*
* Description: Converts the Julian date to regular dates, setting the year, 
*              month, and day ints.  This was copied from 
*              RPCGS_julian_to_date in cpc101/lib004/rpgcs_tim_funcs.c 
*              because we don't want to have to rely on the orpgvcp library.
*
* Inputs: int julian_date - The julian date
*         int* year - A pointer to an int that will hold the year
*         int* month - A pointer to an int that will hold the month
*         int* day - A pointer to an int that will hold the day 
*
* Returns: int - 0
*
*******************************************************************************/
int Julian_to_date(int julian_date, int *year, int *month, int *day)
{
    int julian_1;
    int   l, n;

    /* Convert Julian date to base year of Julian calendar */
    julian_1 = 2440587 + julian_date;

    /* Compute year, month, and day */
    l = julian_1 + 68569;
    n = 4*l/146097;
    l = l -  (146097*n + 3)/4;
    *year = 4000*(l+1)/1461001;
    l = l - 1461*(*year)/4 + 31;
    *month = 80*l/2447;
    *day = l - 2447*(*month)/80;
    l = *month/11;
    *month = *month + 2 - 12*l;
    *year = 100*(n - 49) + (*year) + l;

    return 0;
} /* end Julian_to_date */

/*******************************************************************************
* Function: Julian_to_date
*
* Description: Converts milliseconds in a day to the time, setting the hour, 
*              minute, second, and mills ints.  This was copied from 
*              RPGCS_convert_radial_time in cpc101/lib004/rpgcs_tim_funcs.c 
*              because we don't want to have to rely on the orpgvcp library.
*
* Inputs: unsigned int time - The milliseconds since midnight 
*         int* hour - A pointer to an int that will hold the year
*         int* minute - A pointer to an int that will hold the minute
*         int* second - A pointer to an int that will hold the second
*         int* mills - A pointer to an int that will hold the mills
*
* Returns: int - 0
*
*******************************************************************************/
int Milliseconds_to_time(unsigned int time, int *hour, int *minute, 
                         int *second, int *mills)
{
    int timevalue = time;

    /* Time value must be in the range 0 - 86400000. */
    if( time > 86400000 )
        return(-1);

    /* Extract the number of hours. */
    *hour = timevalue/3600000;

    /* Extract the number of minutes. */
    timevalue -= (*hour)*3600000;
    *minute = timevalue/60000;

    /* Extract the number of seconds. */
    timevalue -= (*minute)*60000;
    *second = timevalue/1000;

    /* Extract the number of milliseconds. */
    *mills = time % 1000;

    /* Put hour, minute, and second in range. */
    if( *hour == 24 )
        *hour = 0;
    if( *minute == 60 )
        *minute = 0;
    if( *second == 60 )
        *second = 0;
    return 0;
} /* end Milliseconds_to_time */

/*******************************************************************************
* Function: uncompress_to_file
*
* Description: This function runs a system call to the 'uncompress' call.
*              Basically, this is used when a user has a .Z file that needs 
*              to be decompressed.  Because there is no library that makes a 
*              call to code that will run the equivalent to "uncompress", we 
*              are just making a system call.  
*
* Inputs: int f_num - The number to append to the temporary file so that
*                       the file is unique.
*         char* filename - The file that we are uncompressing
*
* Returns: int - f_num on success, 0 on failure
*
*******************************************************************************/
static int uncompress_to_file(int f_num, char *filename)
{
    gchar *command = NULL;
    int exit_status;

    /* Create the command line */
    command = g_strdup_printf("/usr/bin/uncompress -c %s > /tmp/uncompress%d", filename, f_num);
    exit_status = system(command);
    g_free(command);
    /* Something went wrong with the actual uncompress */
    if (exit_status == 1)
    {
        fprintf(stderr, "There was a problem with the uncompress.  Please uncompress manually.\n");
        return 0;
    }
    return f_num;
} /* end uncompress_to_file */

/*******************************************************************************
* Function: generate_temp_filename
*
* Description: Because we cannot be assured that a file name already exists
*               in the tmp folder, this one will try to find one.  It simply
*               runs through the name /tmp/uncompressXXXXXX, where the X's are
*               numbers.  Once it finds a file that does not exist, it returns
*               the value of the Xs.
*
* Inputs: none
*
* Returns: int f_num - The number to append to the temporary file so that
*                       the file is unique.
*
*******************************************************************************/
static int generate_temp_filename()
{
    char *tmpF = NULL;
    tmpF = malloc(24);
    memset(tmpF, '\0', 24);
    int i;
    for (i = 0; i < 99999999; i++)
    {
        sprintf(tmpF, "/tmp/uncompress%d", i);
        if (!does_file_exist(tmpF))
        {
            free(tmpF);
            return i;
        }
        memset(tmpF, '\0', 24);
    }
    free(tmpF);
    return -1;
} /* end generate_temp_filename */

/*******************************************************************************
* Function: does_file_exist
*
* Description: This function tests to see if a particular file exists.
*
* Inputs: char* filename - The filename to try
*
* Returns: int - 0 if the file does not exist, 1 if it does
*
*******************************************************************************/
static int does_file_exist(char* filename)
{
    struct stat st;
    if (stat(filename, &st) == -1)
        return 0;
    else
        return 1;
} /* end does_file_exist */

/*******************************************************************************
* Function: is_valid_file_extension
*
* Description: Due to the fact that files can be in so many different 
*               compressed formats, this function attempts to sort things out.
*               If the file ends in a certain file extension, this function
*               will attempt to figure it out, and return an integer
*               reflecting the file extension.
*
* Inputs: char* filename - The filename
*
* Returns: int - An integer reflecting the file extension. 0 if there is none
*                       or if it isn't recognized.  Else, the int should
*                       reflect the value that can be set in the Decompress
*                       variable.
*
*******************************************************************************/
static int is_valid_file_extension(char* filename)
{
    char* fExt = NULL;
    int retVal = 0;
    if (filename)
        fExt = strrchr(filename, '.');
    if (fExt == NULL)
        return 0;
    fExt++;
    if (!g_ascii_strcasecmp(fExt, "z"))
        retVal = 3;
    else if (!g_ascii_strcasecmp(fExt, "bz"))
        retVal = 1;
    else if (!g_ascii_strcasecmp(fExt, "bz2"))
        retVal = 1;
    else if (!g_ascii_strcasecmp(fExt, "bzip"))
        retVal = 1;
    else if (!g_ascii_strcasecmp(fExt, "gz"))
        retVal = 2;
    else if (!g_ascii_strcasecmp(fExt, "gzip"))
        retVal = 2;
    else if (!g_ascii_strcasecmp(fExt, "raw"))
    {
        if (Decompress != 1)
        {
            fprintf(stdout, "\nWARNING: You are using a raw file.");
            fprintf(stdout, "  These files may be compressed with bzip.\n");
            fprintf(stdout, "  If you receive errors, try running again with the --bzip flag.\n\n");
        } /* end Decompress != 1 */
        retVal = 0;
    } /* end else if !g_ascii_strcasecmp(fExt, "raw") */
    return retVal;
} /* end is_valid_file_extension */

/*******************************************************************************
* Function: get_message_num_segs
*
* Description: This function returns the number of segments required 
*               for a particular message.
*
* Inputs: int type - The number of message types.
*
* Returns: int - The number of segments for that type
*
*******************************************************************************/
static int get_message_num_segs(int type)
{
    switch (type)
    {
        case 15:
            return 77;
        case 13:
            return 49;
        case 18:
            return 5;
        default:
            return 1;
    }
} /* end get_message_num_segs */

/*******************************************************************************
* Function: print_msg_hdr
*
* Description: This function prints the msgPrintBuf
*
* Inputs: none
*
* Returns: none
*
*******************************************************************************/
static void print_msg_hdr()
{
    if (msgPrintBuf != NULL)
    {
        print_line_of_stars();
        fprintf(stdout, "*              VALUES - ");
        fprintf(stdout, "%s(%d)\n", packet_type_to_string(curr_type), curr_type);
        print_line_of_stars();
    }
} /* end print_msg_hdr */

/*******************************************************************************
* Function: print_msg_buf
*
* Description: This function prints the msgPrintBuf
*
* Inputs: none
*
* Returns: none
*
*******************************************************************************/
static void print_msg_buf()
{
    if (msgPrintBuf != NULL)
    {
        print_line_of_stars();
        fprintf(stdout, "*              VALUES - ");
        fprintf(stdout, "%s(%d)\n", packet_type_to_string(curr_type), curr_type);
        print_line_of_stars();
        fprintf(stdout, "%s\n", msgPrintBuf);
        g_free(msgPrintBuf);
        msgPrintBuf = NULL;
    }
} /* end print_msg_buf */

/*******************************************************************************
* Function: Print_summary
*
* Description: This function prints the summary at the end of the output.
*
* Inputs: none
*
* Returns: none
*
*******************************************************************************/
static void Print_summary()
{
    fprintf(stdout, "########################################");
    fprintf(stdout, "########################################\n");
    fprintf(stdout, "################################### SUMMARY ");
    fprintf(stdout, "####################################\n");
    fprintf(stdout, "########################################");
    fprintf(stdout, "########################################\n");
    fprintf(stdout, "Radar ID: %s\n", Radar_id);
    int j, yr, mo, day, hr, min, sec, mills;
    Julian_to_date(glob_vol_date, &yr, &mo, &day);
    Milliseconds_to_time(glob_vol_time, &hr, &min, &sec, &mills);
    fprintf(stdout, "The date is: %d:%d:%d\n", yr, mo, day);
    fprintf(stdout, "The time is: %d:%d:%d:%d\n\n", hr, min, sec, mills);

    fprintf(stdout, "File Description:\n");
    for (j = 1; j < 32; j++)
        if (numMsg[j])
            fprintf(stdout, "Number of messages of type %d (%s) - %d\n", 
                    j, packet_type_to_string(j), numMsg[j]);           

    if (summaryBuf != NULL)
        fprintf(stdout, "%s", summaryBuf);
}
