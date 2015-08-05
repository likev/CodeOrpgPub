/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2004/02/05 23:21:31 $
 * $Id: rpg_os32_sysio.c,v 1.3 2004/02/05 23:21:31 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/**************************************************************************

      Module:  os32_sysio.c

 Description:
	This file defines the replacement for the OS32 SYSIO routine.
	The scope of all other routines defined within this file is limited
	to this file.  The private functions are defined in alphabetical
	order, following the definition of the replacement sysio().

	Refer to the product generation and control ORPG documentation for
	descriptions of the various ORPG buffer routines.

	The legacy RPG Rain Gage Database disk file has been replaced by an
	ORPG product GAGEDATA.

	The legacy RPG Hydromet User Selectable Products Internal Data Base
	File has been replaced by an ORPG Linear Buffer file.

        Note that a Unit Test (UT) driver has been provided at the bottom of
        this file.  To compile this source file into a UT binary, compile the
        file with UTMAIN defined and link against the required libraries
        (e.g., libinfr, liborpg, and librpg).  As problems are
        discovered, and/or as new test cases are defined (in the SDF), this
        UT driver must be updated.

 Memory Allocation:
	None beyond that provided by the ORPG support library.

 Assumptions:
	The corresponding ORPG buffers exist and have been registered.

 **************************************************************************/



/*
 * System Include Files/Local Include Files
 */
#include <gagedata.h>
#include <a3147.h>
#include <rpg.h>


#if (defined(SUNOS) || defined(LINUX))
#define sysio sysio_
#endif

/*
 * Constant Definitions/Macro Definitions/Type Definitions
 */
void sysio(fint *pblk,
           fint *fc,
           fint *lu,
           fint *start,
           fint *nbytes,
           fint *ranadd,
           fint *xopt) ;

#define SYSIO_SUCCESS 0
#define SYSIO_FAILURE 1
#define SYSIO_NO_XLATE 0
#define SYSIO_XLATE_ASCII 1
#define SYSIO_READ 0
#define SYSIO_WRITE 1

#define SYSIO_FC_IO_BIT_MASK            0x80
#define SYSIO_FC_READ_BIT_MASK          0x40
#define SYSIO_FC_WRITE_BIT_MASK         0x20
#define SYSIO_FC_XLATE_BIT_MASK         0x10
#define SYSIO_FC_WAIT_BIT_MASK          0x08
#define SYSIO_FC_RACESS_BIT_MASK        0x04

/*
 * Static Globals
 */

/*
 * Static Function Prototypes
 */
static int gagedata_io(int *start,
                       int nbytes,
                       int ranadd) ;

static int hyusrsel_io(int *start,
                       int nbytes,
                       int ranadd,
                       int rw_flag) ;

int process_sysio_fc(int fc, 
                     int *rw_flag, 
                     int *xlate_flag);


/**************************************************************************
 Description: ORPG replacement for the legacy OS32 SYSIO routine.
       Input: (refer to OS32 SYSIO documentation)
              first element of the process block
              function code
              logical unit (ORPG buffer id)
              starting address of buffer for I/O transfer
              number of bytes to be transferred
              logical record number for random access
              extended options argument
      Output: bytes are transferred to/from the specified buffer
              upon success, pblk is set to SYSIO_SUCCESS; otherwise,
              pblk is set to SYSIO_FAILURE
     Returns: none
       Notes:
 **************************************************************************/
void sysio(fint *pblk,
           fint *fc,
           fint *lu,
           fint *start,
           fint *nbytes,
           fint *ranadd,
           fint *xopt)
{
   int retval ;
   int rw_flag ;
   int xlate_flag ;
   static char whoami[] = "sysio" ;

   *pblk = SYSIO_SUCCESS ;


   /*
    * Negative number of I/O bytes is not supported ...
    * The OS32 SVC1 Extended Option is not supported ...
    * Process the SYSIO Function Code ... setting the read/write and
    * ASCII translation flags as required ...
    */
   if (*nbytes < 0) {
      PS_task_abort("%s: *lu %d bad *nbytes: %d\n", whoami, *lu, *nbytes) ;
      *pblk = SYSIO_FAILURE ;
      return ;
   }

   if (*xopt != 0) {
      PS_task_abort("%s: *lu %d nonzero *xopt: 0x%x\n", whoami, *lu, *xopt) ;
      *pblk = SYSIO_FAILURE ;
      return ;
   }

   retval = process_sysio_fc(*fc, &rw_flag, &xlate_flag) ;
   if (retval < 0) {
      PS_task_abort("%s: *lu %d bad *fc: 0x%x\n", whoami, *lu, *fc) ;
      *pblk = SYSIO_FAILURE ;
      return ;
   }
   
   switch(*lu) {

      case GAGEDATA:
         /*
          * We support only random access reads from the three legacy RPG file
          * sectors ...
          */
         if (rw_flag != SYSIO_READ) {
            PS_task_abort("%s: GAGEDATA writes not supported *fc: 0x%x\n",
                          whoami, *fc) ;
            *pblk = SYSIO_FAILURE ;
            return ;
         }

         if ((*ranadd != GAGE_STAT)
                      &&
             (*ranadd != GAGE_IDS)
                      &&
             (*ranadd != GAGE_RPTS)) {

            PS_task_abort("%s: GAGEDATA bad *ranadd: %d\n", whoami, *ranadd) ;
            *pblk = SYSIO_FAILURE ;
            return ;

         }

         retval = gagedata_io(start, *nbytes, *ranadd) ;
         if (retval != NORMAL) {
            PS_task_abort("%s: GAGEDATA fail: %d *nbytes: %d *ranadd: %d\n",
                          whoami, retval, *nbytes, *ranadd) ;
            *pblk = SYSIO_FAILURE ;
         }

         break ;

      case HYUSRSEL:
         /*
          * We support only random access reads/writes from/to the legacy
          * RPG file sectors ...
          */
         if ((*ranadd < (USDB_HDR_SCTR))
                      ||
             (*ranadd > (DFLT_24H_SCTR))) {

            PS_task_abort("%s: HYUSRSEL bad *ranadd: %d\n", whoami, *ranadd) ;
            *pblk = SYSIO_FAILURE ;
            return ;

         }

         /*
          * We support only two sizes of I/O ... either the header message or
          * a one-hour chunk of data ...
          */
         if ((*nbytes != NUM_HDR_BYTES)
                      &&
             (*nbytes != NUM_POLAR_BYTES)) {
            PS_task_abort("%s: HYUSRSEL bad *nbytes: %d\n", whoami, *nbytes) ;
            *pblk = SYSIO_FAILURE ;
            return ;
         }

         retval = hyusrsel_io(start, *nbytes, *ranadd, rw_flag) ;
         if (retval < 0) {
            PS_task_abort("%s: HYUSRSEL fail: %d *nbytes: %d *ranadd: %d\n",
                          whoami, retval, *nbytes, *ranadd) ;
            *pblk = SYSIO_FAILURE ;
         }
         break ;


      default:
         PS_task_abort("%s: unknown *lu: %d\n", whoami, *lu) ;
         *pblk = SYSIO_FAILURE ;
         break ;
   }

   return ;

/*END of sysio()*/
}



/**************************************************************************
 Description: Process the OS32 SYSIO Function Code
       Input: the function code
              pointer to storage for the read/write flag
              pointer to storage for the ASCII translation flag
      Output: read/write flag is set
              ASCII translation flag is set
     Returns: none
       Notes:
 **************************************************************************/
int process_sysio_fc(int fc, int *rw_flag, int *xlate_flag)
{
   static char *whoami = "process_sysio_fc" ;

   /*
    * SYSIO Function Bit 0 must be cleared ...
    */
   if (fc & SYSIO_FC_IO_BIT_MASK) {
      LE_send_msg(GL_ERROR,
                  "%s: SYSIO Function Bit 0 must be cleared.  fc: 0x%x\n",
                  whoami,fc) ;
      return(-1) ;
   }

   /*
    * If SYSIO Function Bit 1 is set, Bit 2 must be cleared ...
    * If SYSIO Function Bit 2 is set, Bit 1 must be cleared ...
    */
   if ((fc & SYSIO_FC_READ_BIT_MASK)
                &&
      ! (fc & SYSIO_FC_WRITE_BIT_MASK)) {
     
      *rw_flag = SYSIO_READ ;
   }
   else if ((fc & SYSIO_FC_WRITE_BIT_MASK)
                &&
      ! (fc & SYSIO_FC_READ_BIT_MASK)) {
     
      *rw_flag = SYSIO_WRITE ;
   }
   else {
      LE_send_msg(GL_ERROR,
                  "%s: SYSIO Function Code not read/write.  fc: 0x%x\n",
                  whoami,fc) ;
      return(-1) ;
   }

   /*
    * SYSIO Function Bit 3 provides translation instruction ...
    */
   if (fc & SYSIO_FC_XLATE_BIT_MASK) {
      *xlate_flag = SYSIO_NO_XLATE ;
   }
   else {
      *xlate_flag = SYSIO_XLATE_ASCII ;
   }

   /*
    * SYSIO Function Bit 4 must be set to indicate blocking I/O ...
    */
   if ( !(fc & SYSIO_FC_WAIT_BIT_MASK)) {
      LE_send_msg(GL_ERROR,
                  "%s: SYSIO Function Bit 4 must be set to indicate blocking I/O.  fc: 0x%x\n",
                  whoami,fc) ;
      return(-1) ;
   }

   /*
    * SYSIO Function Bit 5 must be set to indicate random access ...
    */
   if ( !(fc & SYSIO_FC_RACESS_BIT_MASK)) {
      LE_send_msg(GL_ERROR,
                  "%s: SYSIO Function Bit 5 must be set to indicate random access.  fc: 0x%x\n",
                  whoami,fc) ;
      return(-1) ;
   }

   return(0) ;

/*END of process_sysio_fc()*/
}


/**************************************************************************
 Description: Perform all ORPG Rain Gage Database I/O.  This is currently
              constrained to reading GAGEDATA ... the task that writes
              GAGEDATA will not be legacy code.
       Input: starting address of buffer for I/O transfer
              number of bytes to be transferred
              logical record number for random access
      Output: I/O data are read from GAGEDATA into the provided address
     Returns: NORMAL upon success; otherwise -1 or a meaningful nonzero return
              value from a library routine
       Notes: ranadd is constrained to GAGE_STAT, GAGE_IDS, and GAGE_RPTS.
              This check has already been performed.
 **************************************************************************/
static int gagedata_io(int *start,
                       int nbytes,
                       int ranadd)
{
   char *buf_p ;
   int buflen ;
   gagedata *gagedb_p ;
   int length ;
   LB_info list ;
   int retval = NORMAL ;
   static char *whoami = "gagedata_io" ;

   /*
    * We must accomodate the ORPG Product Header ...
    */
   buflen = (int) (sizeof(Prod_header) + sizeof(gagedata)) ;

   buf_p = (char *) calloc((size_t) 1, (size_t) buflen) ;
   if (NULL == buf_p) {
      LE_send_msg(GL_ERROR,
                  "%s: Unable to allocate memory for Gage Data\n",whoami) ;
      return(-1) ;
   }

   /*
    * Obtain message ID and size of latest message ...
    */
   if ((length = ORPGDA_list(GAGEDATA, &list, 1)) <= 0) {
      LE_send_msg(GL_ERROR,
                  "%s: ORPGDA_list returned %d\n",whoami,length) ;
      free(buf_p) ;
      return(-1);
   }

   if (list.size != buflen) {
      LE_send_msg(GL_ERROR,
                  "%s: list.size %d != buflen %d\n",
                  whoami,list.size, buflen) ;
      free(buf_p) ;
      return(-1) ;
   }

   /*
    * There is no reason why we should accept anything less than the
    * number of bytes we are expecting ...
    */
   if ((length = ORPGDA_read(GAGEDATA, buf_p, buflen, list.id)) != buflen) {
      LE_send_msg(GL_ERROR,
                  "%s: ORPGDA_read returned %d rather than %d\n",
                  whoami,length,buflen) ;
      free(buf_p) ;
      return(-1) ;
   }

   PS_message("%s: read %d GAGEDATA bytes from msg id %d\n",
              whoami,buflen,list.id) ;

   /*
    * We must skip over the ORPG Product Header ...
    */
   gagedb_p = (gagedata *) &buf_p[sizeof(Prod_header)] ;

   if (ranadd == GAGE_STAT) {

      /*
       * Copy nbytes of the Precipitation Status Information ...
       */
      (void) memcpy(start,
                    (const void *) &(gagedb_p->precip_status_info),
                    (size_t) nbytes) ;

      PS_message("%s: copied %d GAGEDATA Precip Status Info bytes\n",
                 whoami,nbytes) ;

   }
   else if (ranadd == GAGE_IDS) {

      /*
       * Copy nbytes of the Rain Gage Identification Information ...
       */
      (void) memcpy(start,
                    (const void *) &(gagedb_p->id_info),
                    (size_t) nbytes) ;

      PS_message("%s: copied %d GAGEDATA Rain Gage ID Info bytes\n",
                 whoami,nbytes) ;

   }
   else if (ranadd == GAGE_RPTS) {

      /*
       * Copy nbytes of the Rain Gage Reports ...
       */
      (void) memcpy(start,
                    (const void *) &(gagedb_p->reports),
                    (size_t) nbytes) ;

      PS_message("%s: copied %d GAGEDATA Rain Gage Reports bytes\n",
                 whoami,nbytes) ;
   }
   else {
      /*
       * This "should not" happen ...
       */
      LE_send_msg(GL_ERROR,
                  "%s: bad ranadd %d\n",whoami,ranadd) ;
      retval = -1 ;
   }

   free(buf_p) ;

   return(retval) ;

/*END of gagedata_io()*/
}


/**************************************************************************
 Description: Perform all ORPG Hydromet User Selectable Products Internal
              Data Base File I/O.
       Input: starting address of buffer for I/O transfer
              number of bytes to be transferred
              logical record number for random access
              read/write flag
      Output: I/O data are read from HYUSRSEL into the provided address
              I/O data are written to HYUSRSEL from the provided address
     Returns: NORMAL upon success; otherwise -1 or a meaningful negative return
              value from support routines
       Notes: ranadd is constrained to the legacy (constant) USDB Sector
              Offset values.  A corresponding bounds check has already been
              performed.  A finer check is performed in this routine.
              The HYUSRSEL Linear Buffer holds 62 messages with message ids
              that correspond to the USDB Sector Offset values.
              Message id 0 (the header) is NUM_HDR_BYTES long, while the
              remaining messages are NUM_POLAR_BYTES long.
 **************************************************************************/
static int hyusrsel_io(int *start,
                       int nbytes,
                       int ranadd,
                       int rw_flag)
{

   register int i ;
   LB_id_t id ;
   static LB_id_t lb_ids[MAX_USDB_RECS] ;
   int retval ;
   static char *whoami = "hyusrsel_io" ;

   /*
    * Some one-time initialization ...
    */
   if (lb_ids[MAX_USDB_RECS - 1] == 0) {

      /*
       * Initialize the array of valid HYUSRSEL Linear Buffer message ids ...
       * These correspond to the constant USDB Sector Offsets values ...
       * This code mimics the USDB_SCTR_OFFS() initialization code found in
       * A31472__INIT_USDB.
       */
      lb_ids[0] = (LB_id_t) USDB_HDR_SCTR ;
      lb_ids[1] = lb_ids[0] + (LB_id_t) NUM_HDR_SCTRS ;

      for (i=2; i < (MAX_USDB_RECS - 1); ++i) {
         lb_ids[i] = lb_ids[i-1] + (LB_id_t) NUM_POLAR_SCTRS ;
      }

      lb_ids[MAX_USDB_RECS - 1] = (LB_id_t) DFLT_24H_SCTR ;

   } /* end of one-time initialization ... */


   /*
    * Verify the rand access address ... should match a well-known
    * USDB Sector Offsets value ...
    */
   id = (LB_id_t) ranadd ;
   for (i=0; i < MAX_USDB_RECS; ++i) {
      if (id == (int) lb_ids[i]) {
         break ;
      }
   }

   if (i >= MAX_USDB_RECS) {
      LE_send_msg(GL_ERROR,
                  "%s: Unable to match random access address %d to USDB Sector Offset\n",
                  whoami,ranadd) ;
      return(-1) ;
   }

   if (id == lb_ids[0]) {
      /*
       * We cannot perform I/O for more bytes than are in the header ...
       */
      if (nbytes != NUM_HDR_BYTES) {
         LE_send_msg(GL_ERROR,
                  "%s: nbytes %d inappropriate for msg id %d\n",
                  whoami,nbytes,(int) id) ;
         return(-1) ;
      }

   }
   else {
      /*
       * We cannot perform I/O for more bytes than are in a one-hour chunk
       * of data ...
       */
      if (nbytes != NUM_POLAR_BYTES) {
         LE_send_msg(GL_ERROR,
                  "%s: incorrect nbytes %d ... expect %d\n",
                  whoami,nbytes,NUM_POLAR_BYTES) ;
         return(-1) ;
      }

   }

   switch(rw_flag) {
      case SYSIO_READ:

         retval = ORPGDA_read(HYUSRSEL, (char *) start, nbytes, id) ;
         if (retval < 0) {
            LE_send_msg(GL_ERROR,
                     "%s: ORPGDA_read returned %d (start: 0x%x nbytes: %d id: %d\n",
                     whoami,retval,start,nbytes,(int)id) ;
            return(retval) ;
         }
         PS_message("%s: read %d HYUSRSEL bytes from msg id %d to 0x%x\n",
                    whoami,nbytes,id,start) ;

         break ;

      case SYSIO_WRITE:

         retval = ORPGDA_write(HYUSRSEL, (char *) start, nbytes, id) ;
         if (retval < 0) {
            LE_send_msg(GL_ERROR,
                     "%s: ORPGDA_write returned %d (start: 0x%x nbytes: %d id: %d\n",
                     whoami,retval,start,nbytes,(int)id) ;
            return(retval) ;
         }
         PS_message("%s: wrote %d HYUSRSEL bytes from 0x%x to msg id %d\n",
                    whoami,nbytes,start,id) ;

         break ;

      default:
         /*
          * This "should not" happen ...
          */
         LE_send_msg(GL_ERROR,
                    "%s: Bad rw_flag %d (should be either %d or %d)\n",
                    whoami,rw_flag,SYSIO_READ,SYSIO_WRITE) ;
         return(-1) ;
         break ;
   }

   return (retval);


/*END of hyusrsel_io()*/
}


#ifdef UTMAIN


#define UT_PASSED       0
#define UT_FAILED       -1
#define UT_WHOAMI "os32_sysio_ut"

/*
 *      Unit Test Cases
 *      UT Case 1.4
 */
static int Ut_case_1_4(void) ;

/**************************************************************************
 Description: This is the built-in Unit Test driver
       Input: void
      Output:
     Returns: EXIT_SUCCESS if all unit tests pass; otherwise EXIT_FAILURE
       Notes: Refer to SDF.
              EXIT_SUCCESS and EXIT_FAILURE exit codes are provided so that
              a shell script can recognize if one or more of the unit tests
              failed without having to examine the displayed data.
 **************************************************************************/
int main(int argc, char **argv)
{

   int exitcode = EXIT_SUCCESS ;
   int retval ;

   /*
    * Execute automated (canned) unit tests ...
    */
   retval = Ut_case_1_4() ;
   if (retval != UT_PASSED) {
      exitcode = EXIT_FAILURE ;
   }



/*END of main()*/
}


/**************************************************************************
                UUT: hyusrsel_io()

 Description: Unit Test Case 1.4 hyusrsel_io Exceptions
       Input: void
      Output: none
     Returns: void
       Notes: Refer to SDF
 **************************************************************************/
static int Ut_case_1_4()
{

   time_t curtime ;
   int expected ;
   int fxn_return = UT_PASSED ;
   register int i ;
   int retval ;
   char subtest_id ;
   static char *utcase = "UT Case 1.4 hyusrsel_io Exceptions" ;
   static char *uut = "hyusrsel_io" ;

   int *start ;
   int nbytes ;
   int ranadd ;
   int rw_flag ;


   curtime = time((time_t *) NULL) ;

   (void) fprintf(stderr,"BEGIN %40s ... %s", utcase, ctime(&curtime)) ;


   /*
    * Subtest a: bad random access address (does not match well-known
    *            USDB Sector Offsets value)
    */
   subtest_id = 'a' ;
   expected = -1 ;
   start = NULL ;
   nbytes = 0 ;
   ranadd = 3 ;
   rw_flag = SYSIO_READ ;
   retval = hyusrsel_io(start, nbytes, ranadd, rw_flag) ;
   if (retval == expected) {
      (void) fprintf(stderr,"\tSubtest %c: PASS\n", subtest_id) ;
   }
   else {
      fxn_return = UT_FAILED ;
      (void) fprintf(stderr,"\tSubtest %c: FAIL\n", subtest_id) ;
      (void) fprintf(stderr,"\t\tEXPECT %20s retval: %d\n",uut,expected) ;
      (void) fprintf(stderr,"\t\tACTUAL %20s retval: %d\n", uut,retval) ;
   }


   /*
    * Subtest b: try to read header, specifying wrong number of bytes
    */
   subtest_id = 'b' ;
   expected = -1 ;
   start = NULL ;
   nbytes = NUM_POLAR_BYTES ;
   ranadd = USDB_HDR_SCTR ;
   rw_flag = SYSIO_READ ;
   retval = hyusrsel_io(start, nbytes, ranadd, rw_flag) ;
   if (retval == expected) {
      (void) fprintf(stderr,"\tSubtest %c: PASS\n", subtest_id) ;
   }
   else {
      fxn_return = UT_FAILED ;
      (void) fprintf(stderr,"\tSubtest %c: FAIL\n", subtest_id) ;
      (void) fprintf(stderr,"\t\tEXPECT %20s retval: %d\n",uut,expected) ;
      (void) fprintf(stderr,"\t\tACTUAL %20s retval: %d\n", uut,retval) ;
   }


   /*
    * Subtest c: try to read one byte too few
    */
   subtest_id = 'c' ;
   expected = -1 ;
   start = NULL ;
   nbytes = NUM_POLAR_BYTES - 1;
   ranadd = USDB_HDR_SCTR + NUM_HDR_SCTRS ;
   rw_flag = SYSIO_READ ;
   retval = hyusrsel_io(start, nbytes, ranadd, rw_flag) ;
   if (retval == expected) {
      (void) fprintf(stderr,"\tSubtest %c: PASS\n", subtest_id) ;
   }
   else {
      fxn_return = UT_FAILED ;
      (void) fprintf(stderr,"\tSubtest %c: FAIL\n", subtest_id) ;
      (void) fprintf(stderr,"\t\tEXPECT %20s retval: %d\n",uut,expected) ;
      (void) fprintf(stderr,"\t\tACTUAL %20s retval: %d\n", uut,retval) ;
   }

   /*
    * Subtest d: try to read one byte too many
    */
   subtest_id = 'd' ;
   expected = -1 ;
   start = NULL ;
   nbytes = NUM_POLAR_BYTES + 1;
   ranadd = USDB_HDR_SCTR + NUM_HDR_SCTRS ;
   rw_flag = SYSIO_READ ;
   retval = hyusrsel_io(start, nbytes, ranadd, rw_flag) ;
   if (retval == expected) {
      (void) fprintf(stderr,"\tSubtest %c: PASS\n", subtest_id) ;
   }
   else {
      fxn_return = UT_FAILED ;
      (void) fprintf(stderr,"\tSubtest %c: FAIL\n", subtest_id) ;
      (void) fprintf(stderr,"\t\tEXPECT %20s retval: %d\n",uut,expected) ;
      (void) fprintf(stderr,"\t\tACTUAL %20s retval: %d\n", uut,retval) ;
   }


   (void) fprintf(stderr,"  END %40s ... %s", utcase, ctime(&curtime)) ;


/*END of Ut_case_1_4()*/
}


#endif /*DO NOT REMOVE!*/
