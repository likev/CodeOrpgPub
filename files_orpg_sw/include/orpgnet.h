/*
 * RCS info
 * $Author: nolitam $
 * $Locker:  $
 * $Date: 2002/12/11 22:10:46 $
 * $Id: orpgnet.h,v 1.5 2002/12/11 22:10:46 nolitam Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/**************************************************************************

      Module: orpgnet.h

 Description: ORPG network connectivity status API

 **************************************************************************/

#ifndef ORPGNET_H
#define ORPGNET_H

#include <le.h>
#include <orpgerr.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*  Connectivity status values */
typedef enum
{
    ORPGNET_UNKNOWN,		/**  @desc "Unknown"  **/
    ORPGNET_NOT_CONNECTED,	/**  @desc "Not Connected"  **/
    ORPGNET_CONNECTED		/**  @desc "Connected"     **/
} orpgnet_status_t;

typedef struct
{
	/**  Machine name of the message or object that changed   */
	char* name;
	/**  User Data passed in on registration */
	void* user_data;
	/*  Connectivity Status of this machine */
	orpgnet_status_t status;
	/*  true if this machine is local, false otherwise */
	int is_local;
} orpgnet_event_t;


/**  Callback function for the ORPGNET_on_status_change function  **/
typedef void (*orpgnet_status_chg_func_ptr_t)(orpgnet_event_t* event);

/**  Iterator used to iterate through the nodes monitored by the ORPGNET api  **/
typedef void* orpgnet_node_iterator_t;

/**   Command to retrieve the nodes that are being monitored  **/
typedef enum
{
	ORPGNET_FIRST_NODE,     /**  @desc "Get First Node"   **/
	ORPGNET_NEXT_NODE,  	/**  @desc "Get Next Node"   **/
} orpgnet_iterate_command_t;

/**  Causes ORPGNET_log_last_error and ORPGNET_last_error_text to report any low level details associated with the error */
#define ORPGNET_REPORT_DETAILS		0x0001

/**  Causes PROP_log_last_error	and PROP_last_error_text to clear the last error message after their work is done */
#define ORPGNET_CLEAR_ERROR		0x0002

/**  Return the working directory path that contains network status messages and
     other temporary files used while monitoring the network.
     @param(out) buf - Buffer to contain the directory path on output
     @param(in) len - length of buf.  Should be > 2
     @returns - 1 if the path was successfuly retrieved. 0 otherwise.
**/
int ORPGNET_get_mnet_dir(char* buf, int len);

/**  Return the path of the network status named message store
	This function should not be used to access network status information name store directly
	The other ORPGNET functions should be used to access network status information instead

	@param path_buf(out) - path of the network status named message store
	@param buf_len(in) - length of path_buf
	@returns - 1 if successful, 0 otherwise
		   On failure, exception information can be retrieved with ORPGNET_last_error_text or
		   logged with ORPGNET_log_last_error
*/
int ORPGNET_status_filepath(char* path_buf, int buf_len);

/**  Refresh/Create the current name store object
	@returns - 1 if successful, 0 otherwise
		   On failure, exception information can be retrieved with ORPGNET_last_error_text or
		   logged with ORPGNET_log_last_error
*/

int ORPGNET_log_last_error(int le_code, int flags);

/**  Return the text associated with the last error message
	@param error_buf(out) - buffer that contains output error text
	@param error_buf_len(in) - length of error_buf
	@param flags(in) - whether or not to report error details - (ORPGNET_REPORT_DETAILS)
	@returns - 1 on success, 0 on failure.  The return value does not need to
		be tested because this function will always log some error information
		even when an exception has not occurred
*/
int ORPGNET_last_error_text(char* error_buf, int error_buf_len, int flags);

/**  Return 1 if an error has occurred, and it has not be cleared by a
	ORPGNET_clear_error or ORPGNET_log_last_error call  */
int ORPGNET_error_occurred();

/**  Clear error information from the last error that occurred using the ORPGNET api  */
void ORPGNET_clear_error();

/**  Register for notification of network status change
	@param name(in) - name of the machine to listen for network changes, "network" indicates listen
			  for changes in network connectivity from the local machine to all other machines
			  NULL indicates listen for connectivity changes on all machines, including status
			  changes in the local machine's connectivity to the outside world
	@param change_callback(in) - function that will be called when the connectivity status of
			  a machine changes
	@param user_data - user_data that will be supplied to the callback function when the event
				occurs (can be NULL)
	@returns 1 on success, 0 on failure
		   On failure, exception information can be retrieved with ORPGNET_last_error_text or
		   logged with ORPGNET_log_last_error

*/
int ORPGNET_on_status_change(const char* name, orpgnet_status_chg_func_ptr_t change_callback, void* user_data);

/**  Get a network status value of type orpgnet_status_t
	@param name(in) -  Name of the network node - can be "network" to check the status of the local node to the outside world
	@returns - On success, returns the connectivity status of the specified node, otherwise return -1
		   On failure, exception information can be retrieved with ORPGNET_last_error_text or
		   logged with ORPGNET_log_last_error
 */
int ORPGNET_get_status(const char* name);

/**  Determine if a machine name is local or not
	@param name(in) -  Name of the network node - can be "network" to check the status of the local node to the outside world
	@returns - On success, returns 1 if the specified machine name is local, 0 if the machine is not local and -1 if
		   an error occurs
		   On failure, exception information can be retrieved with ORPGNET_last_error_text or
		   logged with ORPGNET_log_last_error
 */
int ORPGNET_machine_is_local(const char* name);

/**  Iterate through the network nodes that are being monitored
	The special node "network" that is used to monitor the connectivity of the local machine to the
	outside world is included in the list of nodes

	@param i - iterator indicating the current position within the list of monitored nodes, can be NULL
			for the ORPGNET_FIRST_NODE command
	@param command - command to obtain, advance, or decrement and iterator to reference the appropriate
			 node.  ORPGNET_FIRST_NODE returns an iterator pointing to the first monitored network node.
			 ORPGNET_NEXT_NODE increments iterators to the next network node
	@param free_iterator_at_end - if this is set to 1, the iterator i will be freed when the end of the list
					is encountered, othewise the iterator will not be freed by this routine
	@returns - On success, returns a non-null iterator.  If the end of the iteration is encountered when using the
		   ORPGNET_FIRST_NODE or ORPGNET_NEXT_NODE commands, then a NULL iterator will be returned
		   to indicate the end of the list of monitored network nodes.
		   If a null is returned due to some exceptional condition, exception information can be
		   retrieved with ORPGNET_last_error_text or
		   logged with ORPGNET_log_last_error
 */
orpgnet_node_iterator_t ORPGNET_iterate(orpgnet_node_iterator_t i, orpgnet_iterate_command_t command, int free_iterator_at_end);

/**  Free the resources associated with an iterator created by ORPGNET_iterate
	@param i - iterator indicating the current position within the list of monitored nodes
			should never be NULL

 */
void ORPGNET_destroy_iterator(orpgnet_node_iterator_t i);

/**  Return the name of the node for a particular iteration position within the
	list of monitored network nodes

     @param i(in) - iterator indicating the current position within the list of monitored nodes
			should not ever be NULL
		    	should be an iterator that was created using the ORPGNET_iterate function
			the return value from ORPGNET_iterate should be tested to make sure
			the iterator i is not past the end or before the beginning of the
			list of node names

     @returns - the name associated with the iteration position reprsented by the iterator i
		If i is past the end of the list of monitored network nodes, NULL is returned
		
*/
const char* ORPGNET_node_name(orpgnet_node_iterator_t i);
			



#ifdef __cplusplus
}
#endif

#endif /* #ifndef ORPGNET_H DO NOT REMOVE! */
