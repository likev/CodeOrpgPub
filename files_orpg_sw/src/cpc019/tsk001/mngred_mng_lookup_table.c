/********************************************************************************
 
      file: mngred_mng_lookup_table.c

      This file contains all the routines that manage the LB lookup table. The
      lookup table contains the paired local and redundant channel LB descriptors
      along with the msg_id, datastore id (data_id), redundant channel lb path, 
      the update required flag, and the redundant relevant LB type. A LB that has
      multiple message ids will have a separate table entry for each message 
      for that LB.

      When a callback routine is called requiring that a redundant channel LB needs
      updating, the callback sets the "update required" flag for the applicable
      LB, msg_id in the lookup table. On each pass of the main loop, the table is 
      checked and any LBs that have the "update required" flag set will have its
      message(s) written to the other channel.

 ********************************************************************************/

/*
 * RCS info
 * $Author: garyg $
 * $Locker:  $
 * $Date: 2001/02/08 21:27:21 $
 * $Id: mngred_mng_lookup_table.c,v 1.3 2001/02/08 21:27:21 garyg Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <mngred_globals.h>

/* file scope variable list */

typedef struct       /* lookup table attributes */
{
   Lb_table_entry_t *table_ptr;     /* pointer to the lookup table           */
   void             *table_id;      /* id of the lookup table                */
   int              number_entries; /* number of entries in the lookup table */
}   Lb_table_t;

Lb_table_t Lb_lookup_table = {NULL, NULL, 0}; /* LB lookup table containing 
                                                 all redun relavent LB info  */

/********************************************************************************

    Description: This routine adds entries to the LB lookup table

          Input: data_id    - data id of the local LB
                 lbfd       - the local channel LB descriptor
                 redun_lbfd - redundant channel LB descriptor
                 msg_id     - the id of the msg to update (a msg id of -1 
                              specifies the whole LB is updated)
                 lb_path    - the path/name of the redundant channel LB
                 lbtype     - the LB update type (see mngred.h, section
                              "flags specifying type of data" for valid lb types

         Output:

         Return: 0 on success; -1 on failure

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

int MLT_add_table_entry (int data_id, int lbfd, int redun_lbfd,
                          LB_id_t msg_id, char *lb_path, int lbtype)
{
   Lb_table_entry_t *table_entry; /* pointer to new table entry */
                           
      /* build a new table entry */

   table_entry = MISC_table_new_entry (Lb_lookup_table.table_id, NULL);

   if (table_entry == NULL)
   {
      LE_send_msg (GL_ERROR, "Failure adding new entry to LB lookup table");
      return (-1);
   }
 
      /* update the fields for this entry */
     
   table_entry->data_id = data_id;
   table_entry->local_lb_fd = lbfd;
   table_entry->redundant_lb_fd = redun_lbfd;
   table_entry->msg_id = msg_id;
   table_entry->update_type = lbtype;
   table_entry->update_required = MNGRED_FALSE;
   strcpy (table_entry->redundant_lb_name, lb_path);

   LE_send_msg (MNGRED_OP_VL,
      "     data_id: %d, local_lbd: %d, redun_lbd: %d, msg_id: %d, lb_type: %d",
      data_id, lbfd, redun_lbfd, msg_id, lbtype);
   LE_send_msg (MNGRED_TEST_VL, "        Redundant LB name: %s", 
                table_entry->redundant_lb_name);
   LE_send_msg (MNGRED_TEST_VL, "        # LB table_entries = %d", 
                Lb_lookup_table.number_entries);

   return (0);
}


/********************************************************************************

    Description: This routine checks for any pending LB updates 

          Input: data_type - type data we are inquiring about (State Data
                             or Adaptation Data)

         Output:

         Return: TRUE if any commands are pending; otherwise, FALSE is
                 returned

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

int MLT_are_updates_pending (int data_type)
{
   int i;
   int ret_code = MNGRED_FALSE;   /* return code to the calling routine */
   Lb_table_entry_t *table_entry; /* ptr to table entry to check */


   table_entry = Lb_lookup_table.table_ptr;

      /* check all entries in the lookup table for any redundant 
         channel updates pending */
   
   for (i = 1; i <= Lb_lookup_table.number_entries; i++)
   {
         /* check for any pending State Data updates */

      if (data_type == MNGRED_STATE_DAT)
      {
         if ((table_entry->update_type == MNGRED_TRANSFER_STATE_DATA)  &&
             (table_entry->update_required == MNGRED_TRUE))
         {
               ret_code = MNGRED_TRUE;
               break;
         }
      }
         /* check for any pending Adaptation Data updates */

      else if (data_type == MNGRED_ADAPT_DAT)
      {

         if ((table_entry->update_type == MNGRED_TRANSFER_ADAPT_DATA)  &&
             (table_entry->update_required == MNGRED_TRUE))
         {
               ret_code = MNGRED_TRUE;
               break;
         }
      }
      
      ++table_entry;
   }

   return (ret_code);
}


/********************************************************************************

    Description: This routine clears the "update required" flag for 
                 all entries in the lookup table

          Input:

         Output:

         Return:

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

void MLT_clear_update_required_flags ()
{
   int i;
   Lb_table_entry_t *table_entry;  /* ptr to table entry */

   table_entry = Lb_lookup_table.table_ptr;

      /* clear all update_required flags in the lookup table */
   
   for (i = 1; i <= Lb_lookup_table.number_entries; i++)
   {
      if (table_entry->update_required == MNGRED_TRUE)
         table_entry->update_required = MNGRED_FALSE;

         /* increment pointer to next table entry */

      ++table_entry;
   }

   LE_send_msg (MNGRED_TEST_VL, "Lookup table update_required flags cleared");

   return;
}


/********************************************************************************

    Description: This routine finds an entry in the lookup table using 
                 this channel's data id as the key

          Input: dataid - id of the LB datastore to find
                 msg_id - msg id to find. if the msg id = -1, then find the table 
                          entry based off the data_id only

         Output:

         Return: return_ptr - pointer to the lookup table entry if the lb data 
                              id is found; otherwise, NULL is returned

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/
   
Lb_table_entry_t *MLT_find_table_entry (int dataid, LB_id_t msg_id)
{
   int i;
   Lb_table_entry_t *lb_table_entry;    /* ptr to the lookup table entries */
   Lb_table_entry_t *return_ptr = NULL; /* return ptr to the calling routine */

      /* initialize the entry pointer to the first entry in the table */

   lb_table_entry = Lb_lookup_table.table_ptr;

      /* find the lb file descriptor in the lb table */

   for (i = 1; i <= Lb_lookup_table.number_entries; i++)
   {
      if (((lb_table_entry->data_id == dataid) && (msg_id == -1))
                             ||
         ((msg_id != -1) && (lb_table_entry->msg_id == msg_id) &&
          (lb_table_entry->data_id == dataid)))
      {
         return_ptr = lb_table_entry;
         break;
      }
         /* increment to the next table entry */

      ++lb_table_entry;
   }

   return (return_ptr);
}


/********************************************************************************

    Description: This routine returns the number of entries in the lookup table

          Input:

         Output:

         Return: number of entries in the Lb lookup table

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

int MLT_get_number_of_entries (void)
{
   return (Lb_lookup_table.number_entries);
}


/********************************************************************************

    Description: This routine returns the pointer of the table id to the 
                 calling routine

          Input:

         Output:

         Return: the pointer to the table id

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

void *MLT_get_table_id (void)
{
   return (Lb_lookup_table.table_id);
}


/********************************************************************************

    Description: This routine returns the table pointer to the calling 
                 routine

          Input:

         Output:

         Return: the table pointer

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

Lb_table_entry_t *MLT_get_table_ptr (void)
{
   return (Lb_lookup_table.table_ptr);
}


/********************************************************************************

    Description: This routine opens the LB lookup table. This table is
                 used to look up the local and redundant channel LB 
                 descriptors, redundant LB path names, the LB datastore ids 
                 (data_id), and any other info necessary for the redundant
                 manager to manage LBs on the redundant RPG channel.

          Input:

         Output:

         Return: 0 on success; -1 on failure

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

int MLT_open_lookup_table (void)
{ 
   int keep_table_order = 0;  /* flag specifying to compress the table
                                 on deletions */
   int table_increment = 10;  /* # of entries to increment lookup table
                                 by when table size is increased */

      /* initialize the table */

   Lb_lookup_table.table_id = MISC_open_table (sizeof (Lb_table_entry_t), 
                                    table_increment,
                                    keep_table_order, 
                                    &Lb_lookup_table.number_entries, 
                                    (char **) &Lb_lookup_table.table_ptr); 

   if (Lb_lookup_table.table_id == NULL)
   {
      LE_send_msg (GL_ERROR, 
            "MLT_open_lookup_table:MISC_open_table returned NULL");
      return (-1);
   }
   else
      LE_send_msg (MNGRED_OP_VL, "LB lookup table opened");

   return (0);
}


/********************************************************************************

    Description: This routine closes and resets all redundant channel LB 
                 descriptors defined in the lookup table. 

                 Anytime the channel link is lost, the redundant channel LB 
                 descriptors are assumed to be invalid and are closed and set 
                 to "UNINITIALIZED" in the lookup table. 

          Input:

         Output:

         Return:

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

void MLT_reset_redun_ch_lbds (void)
{
   int i;
   Lb_table_entry_t *table_entry;  /* pointer to a table entry */

   table_entry = Lb_lookup_table.table_ptr;

   for (i = 1; i <= Lb_lookup_table.number_entries; i++)
   {
      LB_close (table_entry->redundant_lb_fd);
      table_entry->redundant_lb_fd = MNGRED_UNINITIALIZED;

      ++table_entry;
   }

   LE_send_msg (MNGRED_TEST_VL, 
                "All redun lbds set to \"UNINITIALIZED\" in lookup table");

   return;
}


/********************************************************************************

    Description: This routine sets the "update_required" flag for a specific
                 LB, msg id in the LB lookup table.

          Input: data_id        - the id of the LB datastore 
                 local_lbd      - the descriptor of the local LB
                 msg_id         - the id of the msg to update
                 lb_update_type - specifies whether one LB should be
                                  updated or all LBs should be updated 
                 lb_type        - specifies which type of LB to update
                                  (state data or adaptation data)

         Output:

         Return: 0 if table entry "update_required" flag was set, or
                 -1 if table entry was not found

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

int MLT_set_update_required_flag (int data_id, int local_lbd, LB_id_t msg_id,
                                   int lb_update_type, int lb_type)
{
   int i;
   Lb_table_entry_t *table_entry;          /* pointer to a table entry */
   int table_entry_updated = MNGRED_FALSE; /* flag specifying that the table
                                              entry was updated */

   table_entry = Lb_lookup_table.table_ptr;

      /* update the table entries relevant to the lb type */

   switch (lb_type)
   {
      case MNGRED_TRANSFER_ADAPT_DATA:
         if (lb_update_type == MNGRED_UPDATE_ALL_LBS)
         {
            for (i = 1; i <= Lb_lookup_table.number_entries; i++)
            {
               if (table_entry->update_type == MNGRED_TRANSFER_ADAPT_DATA)
               {
                  table_entry->update_required = MNGRED_TRUE;
                  table_entry_updated = MNGRED_TRUE;
                  LE_send_msg (MNGRED_TEST_VL, 
                       "Set update required flag for data_id %d, msg_id %d",
                       table_entry->data_id, table_entry->msg_id);
               }
 
                  /* increment to the next table entry */
                 
               ++table_entry;
            }
         }
         else  /* update just one LB */
         {
            for (i = 1; i <= Lb_lookup_table.number_entries; i++)
            {
               if (table_entry->data_id == data_id)
               {
                    /* set the update_required flag for the msg. if a msg_id
                       of -1 was passed as an argument, then set the 
                       update_required flag for all msgs for this datastore */

                  if ((table_entry->msg_id == msg_id) ||
                      ( table_entry->msg_id == -1) ||
                      ( msg_id == -1))
                  {
                     table_entry->update_required = MNGRED_TRUE;
                     table_entry_updated = MNGRED_TRUE;
                     LE_send_msg (MNGRED_TEST_VL, 
                          "Set update required flag for data_id %d, msg_id %d",
                          table_entry->data_id, table_entry->msg_id);
                  }
               }
 
                  /* increment to the next table entry */
                 
               ++table_entry;
            }
         }
         break;

      case MNGRED_TRANSFER_STATE_DATA:
         if (lb_update_type == MNGRED_UPDATE_ALL_LBS)
         {
            for (i = 1; i <= Lb_lookup_table.number_entries; i++)
            {
               if (table_entry->update_type == MNGRED_TRANSFER_STATE_DATA)
               {
                  table_entry->update_required = MNGRED_TRUE;
                  table_entry_updated = MNGRED_TRUE;
                  LE_send_msg (MNGRED_TEST_VL, 
                        "Set update required flag for data_id %d, msg_id %d",
                        table_entry->data_id, table_entry->msg_id);
               }

                  /* increment to the next table entry */
                  
               ++table_entry;
            }
         }
         else  /* update just one LB */
         {
            for (i = 1; i <= Lb_lookup_table.number_entries; i++)
            {
               if ((table_entry->data_id == data_id) && 
                   (table_entry->msg_id == msg_id))
               {
                     table_entry->update_required = MNGRED_TRUE;
                     table_entry_updated = MNGRED_TRUE;
                     LE_send_msg (MNGRED_TEST_VL, 
                           "Set update required flag for data_id %d, msg_id %d",
                           table_entry->data_id, table_entry->msg_id);
                     break;
               }

                  /* increment to the next table entry */
                  
               ++table_entry;
            }
         }
         break;
   }

   if (table_entry_updated == MNGRED_TRUE)
      return (0);
   else
      return (-1);
}


/********************************************************************************

    Description: Update the redundant channel's LB descriptor for the LB
                 datastore specified

          Input: data_id       - the id of the LB datastore to update
                 new_redun_lbd - the new redundant LB descriptor to update 
                                 the table with

         Output:

         Return:

        Globals: Lb_lookup_table - see file scope global section
 
          Notes:
 
 ********************************************************************************/

void MLT_update_table_redun_ch_lbd (int data_id, int new_redun_lbd)
{
   int i;
   Lb_table_entry_t *lb_table_entry;  /* ptr to a table entry */

      /* initialize the entry pointer to the first entry in the table */

   lb_table_entry = Lb_lookup_table.table_ptr;

   for (i = 1; i <= Lb_lookup_table.number_entries; i++)
   {
         /* check all table entries for this data id to see if any entries
            have a different LB descriptor than the new descriptor assigned
            for this data id */
            
      if (lb_table_entry->data_id == data_id) 
      {
           /* if a valid descriptor is found that is different than the
              new descriptor, close the table entry's descriptor */
            
         if ((lb_table_entry->redundant_lb_fd >= 0) &&
             (lb_table_entry->redundant_lb_fd != new_redun_lbd))
               LB_close (lb_table_entry->redundant_lb_fd);
          
            /* update the table entry with the new descriptor */

         lb_table_entry->redundant_lb_fd = new_redun_lbd;
      }
         /* increment to the next table entry */

      ++lb_table_entry;
   }
   return;
}
