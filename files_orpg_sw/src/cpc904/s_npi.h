/*   @(#)s_npi.h	1.1	07 Jul 1998	*/

/*
 * Copyright (c) 1988-1997 Spider Software Limited
 *
 * This Source Code is furnished under Licence, and may not be
 * copied or distributed without express written agreement.
 *
 * All rights reserved.  Made in Scotland.
 *
 * Authors: Alan Robertson, Peter Woodhouse, Duncan Walker, Jim Stewart
 *
 * s_npi.h of snet module
 *
 * SpiderX25
 * @(#)$Id: s_npi.h,v 1.1 2000/02/25 17:14:53 john Exp $
 * 
 * SpiderX25 Release 8
 */

#define NPI_STID	207

/*
 *  Included X.25 Facility Values
 */

#include "x25_proto.h"
 
    typedef struct {
	unsigned char		CONS_call;
	struct extraformat 	facs;
    } x_format_t;
	
/*
 * Primitives that are initiated by the network user.
 */

#define N_CONN_REQ      0
#define N_CONN_RES      1
#define N_DISCON_REQ    2
#define N_DATA_REQ      3
#define N_EXDATA_REQ    4
#define N_INFO_REQ      5
#define N_BIND_REQ      6
#define N_UNBIND_REQ    7
#define N_UNITDATA_REQ  8
#define N_OPTMGMT_REQ   9

/*
 * Primitives that are initiated by the network provider.
 */

#define N_CONN_IND      11
#define N_CONN_CON      12
#define N_DISCON_IND    13
#define N_DATA_IND      14
#define N_EXDATA_IND     15
#define N_INFO_ACK      16
#define N_BIND_ACK      17
#define N_ERROR_ACK     18
#define N_OK_ACK        19
#define N_UNITDATA_IND  20
#define N_UDERROR_IND   21

/*
 * Additional NPI Primitives
 */

#define N_DATACK_REQ    23
#define N_DATACK_IND    24
#define N_RESET_REQ     25
#define N_RESET_IND     26
#define N_RESET_RES     27
#define N_RESET_CON     28

/*
* The following are the events that drive the state machine
*/

/* Initialization events */

#define NE_BIND_REQ     0
#define NE_UNBIND_REQ   1
#define NE_OPTMGMT_REQ  2
#define NE_BIND_ACK     3
#define NE_ERROR_ACK    5
#define NE_OK_ACK1      6
#define NE_OK_ACK2      7
#define NE_OK_ACK3      8
#define NE_OK_ACK4      9

/* Connection-Mode events */

#define NE_CONN_REQ     10
#define NE_CONN_RES     11
#define NE_DISCON_REQ   12
#define NE_DATA_REQ     13
#define NE_EXDATA_REQ   14
#define NE_CONN_IND     16
#define NE_CONN_CON     17
#define NE_DATA_IND     18
#define NE_EXDATA_IND   19
#define NE_DISCON_IND1  21
#define NE_DISCON_IND2  22
#define NE_DISCON_IND3  23
#define NE_PASS_CON     24
#define NE_RESET_REQ    28
#define NE_RESET_RES    29
#define NE_DATACK_REQ   30
#define NE_DATACK_IND   31
#define NE_RESET_IND    32
#define NE_RESET_CON    33

/* Connection-less events */

#define NE_UNITDATA_REQ 25
#define NE_UNITDATA_IND 26
#define NE_UDERROR_IND  27

#define NE_NOEVENTS     36

/*
 *  NPI interface states
 */

#define NS_UNBND        0
#define NS_WACK_BREQ    1
#define NS_WACK_UREQ    2
#define NS_IDLE         3
#define NS_WACK_OPTREQ  4
#define NS_WACK_RRES    5
#define NS_WCON_CREQ    6
#define NS_WRES_CIND    7
#define NS_WACK_CRES    8
#define NS_DATA_XFER    9
#define NS_WCON_RREQ    10
#define NS_WRES_RIND    11
#define NS_WACK_DREQ6   12
#define NS_WACK_DREQ7   13
#define NS_WACK_DREQ9   14
#define NS_WACK_DREQ10  15
#define NS_WACK_DREQ11  16

#define NS_NOSTATES     18

/*
 *  N_ERROR_ACK error return code values
 */

#define NBADADDR        1
#define NBADOPT         2
#define NACCESS         3
#define NNOADDR         5
#define NOUTSTATE       6
#define NBADSEQ         7
#define NSYSERR         8
#define NBADDATA        10
#define NBADFLAG        16
#define NNOTSUPPORT     18
#define NBOUND          19
#define NBADQOSPARAM    20
#define NBADQOSTYPE     21
#define NBADTOKEN       22


/*
 *  N_UDERROR_IND reason codes
 */

#define N_UD_UNDEFINED          10
#define N_UD_TD_EXCEEDED        11
#define N_UD_CONGESTION         12
#define N_UD_QOS_UNAVAIL        13
#define N_UD_LIFE_EXCEEDED      14
#define N_UD_ROUTE_UNAVAIL      15


/*
 *  NPI Originator for Resets and Disconnects
 */

#define N_PROVIDER      0x0100
#define N_USER          0x0101
#define N_UNDEFINED     0x0102

/*
 *  NPI Disconnect & Reset reasons when the originator is the
 *  N_UNDEFINED
 */

#define N_REASON_UNDEFINED     0x0200

/*
 *  NPI Disconnect reasons when the orignator is the N_PROVIDER
 */

#define N_DISC_P                0x0300
#define N_DISC_T                0x0301
#define N_REJ_NSAP_UNKNOWN      0x0302
#define N_REJ_NSAP_UNREACH_P    0x0303
#define N_REJ_NSAP_UNREACH_T    0x0304

/*
 *  NPI Disconnect reasons when the originator is the N_USER
 */

#define N_DISC_NORMAL           0x0400
#define N_DISC_ABNORMAL         0x0401
#define N_REJ_P                 0x0402
#define N_REJ_T                 0x0403
#define N_REJ_INCOMPAT_INFO     0x0406

/*
 * NPI Disconnect reasons when the originator is the N_USER or
 * N_PROVIDER
 */
#define N_REJ_QOS_UNAVAIL_P     0x0305
#define N_REJ_QOS_UNAVAIL_T     0X0306
#define N_REJ_UNSPECIFIED       0x0307

/*
 *  NPI Reset reasons when originator is N_PROVIDER
 */

#define N_CONGESTION            0x0500
#define N_RESET_UNSPECIFIED     0x0501

/*
 *  NPI Reset reasons when originator is N_USER
 */

#define N_USER_RESYNC           0x0600

/*
 *  NPI Disconnect reasons when operating over a PVC
 */

#define N_PVC_LINKDOWN          0x0700    
#define N_PVC_RMTERROR          0x0701
#define N_PVC_USRERROR          0x0702

/*
 *  CONN flags definition; (used in N_conn_req, N_conn_ind, N_conn_res,
 *  and N_Conn_con primitives)
 *
 *  Flags to indicate support of network provider options; (used with
 *  the OPTIONS flags field of N_info_ack primitive)
 */

#define REC_CONF_OPT            0x00000001
#define EX_DATA_OPT             0x00000002

/*  This flag is used with the OPTIONS_flags field of N_info_ack as
 *  well as the OOPTMGMT_flags field of the N_optmgmt_req primitive
 */

#define DEFAULT_RC_SEL          0x00000003

/*
 * BIND_flags; (used with N_bind_req primitive)
 */

#define DEFAULT_LISTENER        0x00000001
#define TOKEN_REQUEST           0x00000002

#define X25_EXT_REQUEST         0x00000100 /* Req to use X.25 extensions */

/*
 *  QOS Parameter Definitions
 */

/*
 * Throughput
 *
 * This parameter is specified for both directions.
 */

  typedef struct {
        int32    thru_targ_value;
        int32    thru_min_value;
  } thru_values_t;

/*
 *  Transit Delay
 */

   typedef struct {
        int32    td_targ_value;
        int32    td_max_value;
   } td_values_t;

/*
 *  Protection Values
 */

   typedef struct {
        int32    protect_targ_value;
        int32    protect_min_value;
   } protection_values_t;

/*
 *  Priority Values
 */

   typedef struct {
        int32    priority_targ_value;
        int32    priority_min_value;
   } priority_values_t;


/*
 *  Types of protection specifications
 */

#define N_NO_PROT               0x00000000
#define N_PASSIVE_PROT          0x00000001
#define N_ACTIVE_PROT           0x00000002
#define N_ACTIVE_PASSIVE_PROT   0x00000003


/*
 *  Cost Selection
 */

#define N_LEAST_EPXPENSIVE      0x00000000

/*
 *  QOS STRUCTURE TYPES AND DEFINED VALUES
 */

#define N_QOS_CO_RANGE         0x0101
#define N_QOS_CO_SEL           0x0102
#define N_QOS_CL_RANGE         0x0103
#define N_QOS_CL_SEL           0x0104
#define N_QOS_CO_OPT_RANGE     0x0105
#define N_QOS_CO_OPT_SEL       0x0106

#define N_QOS_CO_X25_RANGE     0x0107
#define N_QOS_CO_X25_SEL       0x0108
#define N_QOS_CO_X25_OPT_SEL   0x0109

/*
 *  When a NS user/provider cannot determine the value of a QOS field,
 *  it should return a value of QOS UNKNOWN>
*/

#define QOS_UNKNOWN     -1

/*
 *  QOS range for CONS.  (Used with N_CONN_REQ and N_CONN_IND.)
 */
typedef struct {
        uint32                  n_qos_type;
        thru_values_t           src_throughput_range;
        thru_values_t           dest_throughput_range;
        td_values_t             transit_delay_range;
        protection_values_t     protection_range;
        priority_values_t       priority_range;
} N_qos_co_range_t;

/*
 *  QOS selected for CONS.  (Used with N_CONN_RES and N_CONN_CON.)
 */

typedef struct {
        uint32           n_qos_type;
        int32            src_throughput_sel;
        int32            dest_throughput_sel;
        int32            transit_delay_sel;
        int32            protection_sel;
        int32            priority_sel;
} N_qos_co_sel_t;


/*
 *  QOS and X.25 facilities range for CONS. (Used with N_CONN_REQ and N_CONN_IND.)
 */
 
typedef struct {
        uint32                  n_qos_type;
        thru_values_t           src_throughput_range;
        thru_values_t           dest_throughput_range;
        td_values_t             transit_delay_range;
        protection_values_t     protection_range;
        priority_values_t       priority_range;
	x_format_t		x_format;
} N_qos_co_x25_range_t;

/*
 *  QOS and X25 facilities selected for CONS . (Used with N_CONN_RES and N_CONN_CON.)
 */

typedef struct {
        uint32            n_qos_type;
        int32             src_throughput_sel;
        int32             dest_throughput_sel;
        int32             transit_delay_sel;
        int32             protection_sel;
        int32             priority_sel;
	x_format_t        x_format;
} N_qos_co_x25_sel_t;

/*
 *  QOS range for CLNS options management.  (Used with a N_INFO_ACK.)
 */

typedef struct {
        uint32                  n_qos_type;
        td_values_t             transit_delay_max;
        uint32                  residual_error_rate;
        protection_values_t     protection_range;
        priority_values_t       priority_range;
        int32                   max_accept_cost;
} N_qos_cl_range_t;

/*
 *  QOS selection for CLNS options management.  (Used with
 *  N_OPTMGMT_REQ and N_INFO_ACK.)
 */

typedef struct {
        uint32    n_qos_type;
        int32     transit_delay_max;
        uint32    residual_error_rate;
        int32     protection_sel;
        int32     priority_sel;
        int32     max_accept_cost;
} N_qos_cl_sel_t;

/*
 *  QOS range for CONS options management.
 *  Used with N_OPTMGMT_REQ.)
 */

typedef struct {
        int32                   n_qos_type;
        thru_values_t           src_throughput;
        thru_values_t           dest_throughput;
        td_values_t             transit_delay_t;
        int32                   nc_estab_delay;
        int32                   nc_estab_fail_prob;
        int32                   residual_error_rate;
        int32                   xfer_fail_prob;
        int32                   nc_resilience;
        int32                   nc_rel_delay;
        int32                   nc_rel_fail_prob;
        protection_values_t     protection_range;
        priority_values_t       priority_range;
        int32                   max_accept_cost;
} N_qos_co_opt_range_t;


/*
 *  QOS values selected for CONS options management.
 *  (Used with N_OPTMGMT_REQ and N_INFO_ACK.)
 */

typedef struct {
        int32            n_qos_type;
        thru_values_t    src_throughput;
         thru_values_t   dest_throughput;
        td_values_t      transit_delay;
        int32            nc_estab_delay;
        int32            nc_estab_fail_prob;
        int32            residual_error_rate;
        int32            xfer_fail_prob;
        int32            nc_resilience;
        int32            nc_rel_delay;
        int32            nc_rel_fail_prob;
        int32            protection_sel;
        int32            priority_sel;
        int32            max_accept_cost;
} N_qos_co_opt_sel_t;


/*
 *  QOS and X25 values selected for CONS options management.
 *  (Used with N_OPTMGMT_REQ and N_INFO_ACK.)
 */

typedef struct {
        uint32           n_qos_type;
        thru_values_t    src_throughput;
        thru_values_t    dest_throughput;
        td_values_t      transit_delay;
        int32            nc_estab_delay;
        uint32           nc_estab_fail_prob;
        uint32           residual_error_rate;
        uint32           xfer_fail_prob;
        uint32           nc_resilience;
        int32            nc_rel_delay;
        uint32           nc_rel_fail_prob;
        int32            protection_sel;
        int32            priority_sel;
        int32            max_accept_cost;
	x_format_t	x_format;
} N_qos_co_x25_opt_sel_t;


/*
 *  NPI Primitive Definitions
 */


/*
*  Local management service primitives
*/

/*
 *  Information request
 */
        typedef struct {
                uint32 PRIM_type;
        } N_info_req_t;

/*
 *  Information acknowledgement
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 NSDU_size;
                uint32 ENSDU_size;
                uint32 CDATA_size;
                uint32 DDATA_size;
                uint32 ADDR_size;
                uint32 ADDR_length;
                uint32 ADDR_offset;
                uint32 QOS_length;
                uint32 QOS_offset;
                uint32 QOS_range_length;
                uint32 QOS_range_offset;
                uint32 OPTIONS_flags;
                uint32 NIDU_size;
                int32  SERV_type;
                uint32 CURRENT_state;
                uint32 PROVIDER_type;
        } N_info_ack_t;

/*
 *  Service types supported by NS provider
 */

#define N_CONS  1
#define N_CLNS  2

/*
 *  Valid provider types
 */
#define N_SNICFP        1
#define N_SUBNET        2

/*
 * Bind request
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 ADDR_length;
                uint32 ADDR_offset;
                uint32 CONIND_number;

                uint32 BIND_flags;
        } N_bind_req_t;


/*
 *  Bind acknowledgement
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 ADDR_length;
                uint32 ADDR_offset;
                uint32 CONIND_number;
                uint32 TOKEN_value;
        }  N_bind_ack_t;


/*
 *  Unbind request
 */

        typedef struct {
                uint32 PRIM_type;
        } N_unbind_req_t;

/*
 *  Options management request
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 QOS_length;
                uint32 QOS_offset;
                uint32 OPTMGMT_flags;
        } N_optmgmt_req_t;


/*
 *  Error acknowledgement for CONS services
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 ERROR_prim;
                uint32 NPI_error;
                uint32 UNIX_error;
        } N_error_ack_t;

/*
 *  Successful completion acknowledgement
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 CORRECT_prim;
        } N_ok_ack_t;

/*
 *  CONS PRIMITIVES
 */


/*
 *  Network connection request
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 DEST_length;
                uint32 DEST_offset;
                uint32 CONN_flags;
                uint32 QOS_length;
                uint32 QOS_offset;
        } N_conn_req_t;


/*
 *  Connection indication
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 DEST_length;
                uint32 DEST_offset;
                uint32 SRC_length;
                uint32 SRC_offset;
                uint32 SEQ_number;
                uint32 CONN_flags;
                uint32 QOS_length;
                uint32 QOS_offset;
        } N_conn_ind_t;



/*
 *  Connection response
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 TOKEN_value;
                uint32 RES_length;
                uint32 RES_offset;
                uint32 SEQ_number;
                uint32 CONN_flags;
                uint32 QOS_length;
                uint32 QOS_offset;
        } N_conn_res_t;

/*
 *  Connection confirmation
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 RES_length;
                uint32 RES_offset;
                uint32 CONN_flags;
                uint32 QOS_length;
                uint32 QOS_offset;
        } N_conn_con_t;


/*
 *  Connection mode data transfer request
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 DATA_xfer_flags;
        } N_data_req_t;

/*
 * NPI MODE_DATA_FLAG for segmenting NSDU into more than 1 NIDUs
 */

#define N_MORE_DATA_FLAG	1

/*
 * EXTRA!!! Qual flag for Q bit
 */

#define N_QUAL_FLAG	4

/*
 *  NPI Receipt confirmation request set flag
 */

#define N_RC_FLAG       0x00000002

/*
 *  Address length on a 4 byte boundary
 *  x25_proto.h is included at the top of this file.
 */
#define snpi_addrsz(a)	((sizeof(struct xaddrf) + 4) & ~3)



/*
 *  Incoming data indication for an NC
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 DATA_xfer_flags;
        } N_data_ind_t;


/*
 *  Data acknowledgement request
 */

        typedef struct {
                uint32 PRIM_type;
        }N_datack_req_t;

/*
 *  Data acknowledgement indication
 */

        typedef struct {
                uint32 PRIM_type;
        } N_datack_ind_t;

/*
 *  Expedited data transfer request
 */

        typedef struct {
                uint32 PRIM_type;
        } N_exdata_req_t;


/*
 *  Expedited data transfer indication
 */

        typedef struct {
                uint32 PRIM_type;
        } N_exdata_ind_t;


/*
 *  NC reset request
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 RESET_reason;
        } N_reset_req_t;


/*
 *  NC reset indication
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 RESET_orig;
                uint32 RESET_reason;
        } N_reset_ind_t;

/*
 *  NC reset response
 */
        typedef struct {
                uint32 PRIM_type;
        } N_reset_res_t;


/*
 *  NC reset confirmed
 */

        typedef struct {
                uint32 PRIM_type;
        } N_reset_con_t;

/*
 *  NC disconnection request
 */
        typedef struct {
                uint32 PRIM_type;
                uint32 DISCON_reason;
                uint32 RES_length;
                uint32 RES_offset;
                uint32 SEQ_number;
        } N_discon_req_t;


/*
 *  NC disconnection indication
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 DISCON_orig;
                uint32 DISCON_reason;
                uint32 RES_length;
                uint32 RES_offset;
                uint32 SEQ_number;
        } N_discon_ind_t;


/*
 *  CLNS PRIMITIVES
 */

/*
 *  Unitdata transfer request
 */

       typedef struct {
                uint32 PRIM_type;
                uint32 DEST_length;
                uint32 DEST_offset;
                uint32 RESERVED_field[2];
        } N_unitdata_req_t;


/*
*  Unitdata transfer indication
*/

        typedef struct {
                uint32 PRIM_type;
                uint32 DEST_length;
                uint32 DEST_offset;
                uint32 SRC_length;
                uint32 SRC_offset;
                uint32 RESERVED_field;
        } N_unitdata_ind_t;

/*
 *  Unitdata error indication for CLNS services
 */

        typedef struct {
                uint32 PRIM_type;
                uint32 DEST_length;
                uint32 DEST_offset;
                uint32 RESERVED_field;
                uint32 ERROR_type;
        } N_uderror_ind_t;



/*
 *  The following represents a union of all the NPI primitives
 */

    union N_primitives {
        uint32 type;
        N_info_req_t            info_req;
        N_info_ack_t            info_ack;
        N_bind_req_t            bind_req;
        N_bind_ack_t            bind_ack;
        N_unbind_req_t          unbind_req;
        N_optmgmt_req_t         optmgmt_req;
        N_error_ack_t           error_ack;
        N_uderror_ind_t         uderror_ind;
        N_ok_ack_t              ok_ack;
        N_conn_req_t            conn_req;
        N_conn_ind_t            conn_ind;
        N_conn_res_t            conn_res;
        N_conn_con_t            conn_con;
        N_data_req_t            data_req;
        N_data_ind_t            data_ind;
        N_datack_req_t          datack_req;
        N_datack_ind_t          datack_ind;
        N_exdata_req_t          exdata_req;
        N_exdata_ind_t          exdata_ind;
        N_reset_req_t           reset_req;
        N_reset_ind_t           reset_ind;
        N_reset_res_t           reset_res;
        N_reset_con_t           reset_con;
        N_discon_req_t          discon_req;
        N_discon_ind_t          discon_ind;
        N_unitdata_req_t        unitdata_req;
        N_unitdata_ind_t        unitdata_ind;
};
