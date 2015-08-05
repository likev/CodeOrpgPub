void A31137__HARI_KIRI ();
void A31143__ABORT_REMAINING_VOLSCAN ();
void A31145__ABORT_ME ();
void A31168__ABORT_ME_BECAUSE (int REASON);
void A31169__ABORT_DATANAME_BECAUSE (char dataname[32], int REASON);
void A31169__ABORT_DATATYPE_BECAUSE (int *DATATYPE, int REASON);
void A31210__CHECK_DATA (int *DATATYPE, int *OPSTAT);
void A31211__GET_INBUF (int *REQDATA, int *BUFPTR, int *DATATYPE, int *OPSTAT);
void A31211__GET_INBUF_BY_NAME (char reqname[32], int *BUFPTR, int *DATATYPE, int *OPSTAT);
void A31212__REL_INBUF (int *BUFPTR);
void A31215__GET_OUTBUF (int *DATATYPE, int *BUFSIZE, int *BUFPTR, int *OPSTAT);
void A31215__GET_OUTBUF_BY_NAME (char dataname[32], int *BUFSIZE, int *BUFPTR, int *OPSTAT);
void A31215__GET_OUTBUF_BY_NAME_FOR_REQ (char dataname[32], int *BUFSIZE, int *BUFPTR, short user_array[UA_NUM_FIELDS], int *OPSTAT);
void A31215__GET_OUTBUF_FOR_REQ (int *DATATYPE, int *BUFSIZE, int *BUFPTR, short user_array[UA_NUM_FIELDS], int *OPSTAT);
void A31216__REL_OUTBUF (int *BUFPTR, int *DISPOSITION);
int A31218__BUF_VOL (int *BUFPTR);
int A31219__BUF_ELEV (int *BUFPTR);
void A31238__DONE_SET ();
void A31461__BUFFER_CONTROL (int PARAM);  /* prcpprod */
void A31463__SCAN_TO_SCAN (int hydrmesg[HYZ_MESG], short adjscan[MAX_AZMTHS][MAX_ADJBINS], float hydradap[HYZ_ADAP], int hydrsupl[HYZ_SUPL], int VSNUM);  /* prcpprod */
void A31464__POLAR_TO_CARTESIAN (short polar_grid[KRADS][KBINS], int cartgrid[KGRID_SIZE][KGRID_SIZE], short denom[KGRID_SIZE][KGRID_SIZE], int *MX_CARTVAL);  /* prcpprod */
void A31465__UPDATE_HRLYDB (int supl[HYZ_SUPL], short adjhrly[MAX_AZMTHS][MAX_ADJBINS]);  /* prcpprod */
void A31466__PRODUCT_HEADER (short *prodbuf, int VSNUM, int PRODCODE, int MAXVAL, int hydrsupl[HYZ_SUPL], int LYR3EN);  /* prcpprod */
void A31467__LFM_CART (short adjhrly[MAX_AZMTHS][MAX_ADJBINS], int lfm_grid40[HYZ_LFM40 * HYZ_LFM40], short denom_grd[HYZ_LFM40 * HYZ_LFM40], int hydrsupl[HYZ_SUPL], int *MAX_LFMVAL);  /* prcpprod */
void A31468__PDB_IO (int WORKCODE, int RECNO, short *buf, int IOSTATUS);  /* prcpprod */
void A3146A__INIT_POLAR_TO_CARTESIAN (short test_grid[KGRID_CENTER][KGRID_CENTER]);  /* prcpprod */
void A3146B__PROD81_1HR_DIG_ARRAY (short adjhrly[MAX_AZMTHS][MAX_ADJBINS], float hydradap[HYZ_ADAP], int hydrsupl[HYZ_SUPL], int VSNUM, int lfmgrid_40[HYZ_LFM40][HYZ_LFM40], short denom_grd[HYZ_LFM40][HYZ_LFM40], int *P81STAT);  /* prcpprod */
void A3146E__STORM_TOTAL (short adjscan[MAX_AZMTHS][MAX_ADJBINS], int hydrsupl[HYZ_SUPL], int strmtot[INDICES]);  /* prcpprod */
void A3146F__INIT_FILE ();  /* prcpprod */
void A3146G__BLOCK3_HEADER (short *prodbuf, int *B3HDROFF, int B3STIDX, int PRODCODE, int VSNUM, int SB_VALUE);  /* prcpprod */
void A3146H__PROD79_3HR_RLE (short *prodbuf, int *RLENI2, int *IERR);  /* prcpprod */
void A3146I__UPDATE_DPA_DB (int hydrsupl[HYZ_SUPL], short lfm4grd[HYZ_LFM4][HYZ_LFM4]);  /* prcpprod */
void A3146J__PROD79_BLOCK3 (short *prodbuf, int VSNUM, int ERRCODE, int BLK3_I2OFF, int *B3ENIDX);  /* prcpprod */
void A3146K__PROD81_LAYER1_RLE (int *STAT40, int lfm_grid40[NCOL40 * NROW40], short *prodbuf, int START_RLE40, int *LYR3ST, int MAXIND, int ERRCODE, int VSNUM);  /* prcpprod */
void A3146M__DETERMINE_SCANS (int hydrsupl[HYZ_SUPL], int *NWORDS);  /* prcpprod */
void A3146O__MISSING_TIMES (int *BUF_INDX, short *pbuff);  /* prcpprod */
void A3146Q__GET_SUM (short polar1[MAX_AZMTHS][MAX_ADJBINS], short polar2[MAX_AZMTHS][MAX_ADJBINS], float BIAS);  /* prcpprod */
void A3146R__GET_RATESCANS (int START_INDX, int *NSCANS, int *END_INDX, short *buff);  /* prcpprod */
void A3146S__FORMAT_BLOCK3 (short *prodbuf, int VSNUM, int PRODCODE, float hydradap[HYZ_ADAP], int hydrsupl[HYZ_SUPL], int *ERRCODE, int B3STIDX, int *B3ENIDX);  /* prcpprod */
void A3146T__CNVTIME (int SECONDS, int JULIAN, char char_date_time[14]);  /* prcpprod */
void A3146U__PROD81_LAYER3_ASCII (short *prodbuf, int VSNUM, float hydradap[HYZ_ADAP], int hydrsupl[HYZ_SUPL], int LYR3ST, int *LYR3EN, int ERRCODE);  /* prcpprod */
void A3146V__PROD82_SUPPLEMENTAL (int hydrsupl[HYZ_SUPL], int VSNUM);  /* prcpprod */
void A3146W__PROD82_BLD_PRODBUF (int hydrsupl[HYZ_SUPL], short *prodbuf, int VSNUM, int *ENIDX);  /* prcpprod */
void A3146Z__DPA_SUPPL_DATA (int *BUF_INDX, short *pbuff);  /* prcpprod */
void A31471__DEFINE_USDB ();  /* prcpprod */
void A31472__INIT_USDB ();  /* prcpprod */
void A31473__CHECK_USDB (int TIME_DIF);  /* prcpprod */
void A31474__ROTATE_INDICES ();  /* prcpprod */
void A31475__UPDATE_USDB (int VSNUM, int supl[HYZ_SUPL], short adjhrly[MAX_AZMTHS][MAX_ADJBINS]);  /* prcpprod */
void A31476__COPY_SUPL (int hydrsupl[HYZ_SUPL]);  /* prcpprod */
void A31482__HOURLY_PRODUCTS (int VSNUM, short adjhrly[MAX_AZMTHS][MAX_ADJBINS], float hydradap[HYZ_ADAP], int hydrsupl[HYZ_SUPL], int FLAG78, int FLAG79, int FLAG81);  /* prcpprod */
void A31489__3HOUR_PROD79 (int VSNUM, int *P79STAT, int hydrsupl[HYZ_SUPL]);  /* prcpprod */
void A3148A__GET_RLE_SIZE (short *inbuf, int *RLE_SIZE);  /* prcpprod */
void A3148D__1HOUR_PROD78 (short adjhrly[MAX_AZMTHS][MAX_ADJBINS], float hydradap[HYZ_ADAP], int hydrsupl[HYZ_SUPL], int VSNUM, int *P78STAT);  /* prcpprod */
void A3148N__BUILD_3HOUR ();  /* prcpprod */
void A3148P__BIAS_ARRAY (float SCALING, int *MAXVAL, short in_data[MAX_AZMTHS][MAX_ADJBINS], short out_prod[MAX_AZMTHS][MAX_ADJBINS]);  /* prcpprod */
void A3148T__PROD82_BTABL (int hydrsupl[HYZ_SUPL], short *prodbuf, int *STARTIDX, int *NPAGES);  /* prcpprod */
void A3148U__PROD81_APPEND_BIAS (int hydrsupl[HYZ_SUPL], int *LYR3EN, short *prodbuf);  /* prcpprod */
void A3148V__PROD81_APPEND_ADAP (float hydradap[HYZ_ADAP], int *LYR3EN, short *pbuff);  /* prcpprod */
void A3148X__PROD82_SUP (int hydrsupl[HYZ_SUPL], int VSNUM, short *prodbuf, int *STARTIDX, int *NPAGES);  /* prcpprod */
void A31491__DIGSTM_PRODUCT_CONTROL (int hydrmesg[HYZ_MESG], float hydradap[HYZ_ADAP], int hydrsupl[HYZ_SUPL], int REASONKODE, int OPSTAT1, int VSNUM, short stminfo[MAX_AZMTHS][MAX_ADJBINS]);  /* prcpprod */
void A31492__GET_DLSCALE_MAX (int *MAXVAL, int *SCALE_FACTOR, short in_data[MAX_AZMTHS][MAX_ADJBINS], short out_data[MAX_AZMTHS][MAX_ADJBINS]);  /* prcpprod */
void A31493__DIG_RADIAL (short stminfo_linear[MAX_ADJBINS * MAX_AZMTHS], short *prodbuf, int *LYR1EN);  /* prcpprod */
void A31494__APPEND_ASCII (int hydrmesg[HYZ_MESG], float hydradap[HYZ_ADAP], int supl_pre[SSIZ_PRE], int LYR1EN, short *pbuff);  /* prcpprod */
void A31495__PRODUCT_HDR (short *prodbuf, int VSNUM, int PRODCODE, int MAXVAL, int SCALE_DL, int hydrsupl[HYZ_SUPL]);  /* prcpprod */
void A3CM01__RUN_LENGTH_ENCODE (int START, int DELTA, short *inbuff, int STARTIX, int ENDIX, int NUMBUFEL, int BUFFSTEP, int CLTABIND, int *NRLEB, int BUFFIND, short *outbuff);
void A3CM02__WINDOW_EXTRACTION (float RADIUS_CENTER, float AZIMUTH_CENTER, float LENGTH_KM, float *MAX_RAD, float *MIN_RAD, float *MAX_THETA, float *MIN_THETA);
void A3CM04__FULL_NAME (char *file, char *acct, char *full_name);
void A3CM08__CHANGE_NOTICE (int OLD_QP, int P1, int P2, int P3, int P4, int P5);
char A3CM09__MY_NAME ();
void A3CM15__MAXI (float array[5], short THREE, short FIVE, float *MAX);
void A3CM16__MINI (float array[5], short THREE, short FIVE, float *MIN);
void A3CM17__THETA (float x[5], float y[5], float theta[5], float *MAX_THETA, float *MIN_THETA, short FLAG);
void A3CM20__INTERPOLATE_EWTAB ();
void A3CM22__RASTER_RUN_LENGTH (int NROWS, int NCOLS, int *BUFSTAT, short *inbuff, int CLTABIND, short *outbuff, int OBUFFIND, int *ISTAR2S, int MAXIND);
void A3CM24__GET_CUST_INFO (int *ELIX, int NTRCOD, short user_array[*][10], int *NUMREQ, int *STAT);
void A3CM25__GET_DATE_TIME (int *CURR_DATE, int *CURR_TIME);
void A3CM27__PADFRONT (int STARTIX, int BSTEP, short *outbuf, int OIDX, int *PADCNT, int *STRTDECR);
void A3CM28__PADBACK (int BYTEFLG, short *outbuf, int BSTEP, int PBUFFIND, int IENDIX, int NUMBINS, int *FINALIDX);
void A3CM29__GRID_VECTORS (int OPT, short *obuf, int ONDX, int *BLNGTH);
void A3CM30__ACROSS_VECTORS ();
void A3CM31__DOWN_VECTORS ();
void A3CM32__VAR_DOWN_VECTORS (int OPT);
void A3CM33__STM_ADAPT (float *rbuf, int *ibuf, short *obuf, int IDX, int *EIDX, float FILKERSZ, float FRACTREQ, int FILTERON);
void A3CM35__CHANGE_WXMODE (int NEWWMODE);
void A3CM36__BUILD_CHARLINES (int CAT_NUM_STORMS, int cat_feat[CAT_MXSTMS][CAT_NF], float comb_att[CAT_MXSTMS][CAT_DAT], int NUM_FPOSITS, float forcst_posits[CAT_MXSTMS][MAX_FPOSITS][FOR_DAT], char charline[CAT_MXSTMS +1][72]);
void A3CM37__COMBATTR_TO_PRODBUF (short *gridbuf, int GRIDLNG, short int_line[CAT_MXSTMS +INCRMNT][LINELNG], int CAT_NUM_STORMS, short *catbuf, int *CAT_LNG, int MAX_PAGES);
void A3CM38__JULIAN2DATE (short JULIAN, short date[DATELEN]);
void A3CM39__EXTRACT_RADSTAT (short bdradhdr[PHEDSIZE], int *GOOD_RAD, int *RAD_STAT);
void A3CM3A__SEG_ADAPT (float *rbuf, int *ibuf, short *obuf, int IDX, int *EIDX);
void A3CM3B__COMP_ADAPT (float *rbuf, int *ibuf, short *obuf, int IDX, int *EIDX);
void A3CM3C__PAF_ADAPT (float *rbuf, int *ibuf, short *obuf, int IDX, int *EIDX, float FILKERSZ, float FRACTREQ, int FILTERON);
void A3CM53__WHAT_MOMENTS (short *radial, int *REFFLAG, int *VELFLAG, int *SWFLAG);
void A3CM54__CUST_PROD_MEMSHED (short CUST_INDX, short COPY, int BUFSTAT);
void A3CM56__GET_ELEV_ANGLE (int *VCP, int *ELVNUM, int *ELVANG, int *FOUND);
int A3CM57__VCP_ELEV (int ELEVINDEX, int VCPNUMBER);
void A3CM58__NUM_RAD_BINS (int *RADIALPTR, int MAXBINS, int *NUMBINS, int RADSTEP, int WAVE_TYPE);
void A3CM59__MAX_BIN (int *RADIALPTR, int MAXBINS, int *NUMBINS, int RADSTEP, int WAVE_TYPE, float *ELEVANG);
void A3CM60__INVERT_MATRIX (float a[n *n], int N, float *DET, int l[n], int m[n]);
void A3CM70__REPORT_ERROR (char *msg);
void ADE_initialize ();
int ADE_update_ade (int new_vol);
int ADE_update_ade_by_event (en_t event_id);
void ADP_initialize ();
void ADP_update_adaptation (int new_vol);
void ADP_update_adaptation_by_event (en_t event_id);
int AP_abort_flag (int abort_flag);
int AP_abort_outputs (int reason);
int AP_abort_request (Prod_request *pr, int reason);
int AP_abort_single_output (int datatype, int reason);
void AP_add_output_to_prod_list (Prod_header *phd);
int AP_alg_control (int alg_control);
int AP_get_abort_reason ();
void AP_hari_kiri ();
void AP_init_abort_reason ();
void AP_initialize ();
void AP_set_abort_reason (int reason);
int ES_event_registered (int event_code);
void IB_check_input_buffer_load ();
void IB_get_id_from_name (char *data_name, int *data_id);
int IB_get_inbuf (int *mem, int *reqdata, int *bufptr, int *datatype, int *opstat);
void IB_initialize (Orpgtat_entry_t *task_table);
int IB_inp_list (In_data_type **ilist);
void IB_new_session ();
LB_id_t IB_product_database_query (int ind);
int IB_read_driving_input (int *buffer_ind);
void IB_reg_inputs (int *status);
int IB_rel_all_inbufs ();
int IB_rel_inbuf (int *bufptr);
void IB_release_input_buffer (int ind, int buffer_ind);
void IB_set_task_name (char *task_name);
int INIT_get_task_input_stream ();
int INIT_process_argv (char *argv, int *len);
int INIT_process_eov_event ();
void INIT_register_sc_status (int *sc_changed);
void INIT_set_arguments (int argc, char *argv[]);
char *INIT_task_name ();
int INIT_task_terminating ();
int INIT_task_type ();
void *ITC_get_data (int itc_id);
void ITC_initialize ();
void ITC_read_all (int inp_prod);
void ITC_update (int new_vol);
void ITC_update_by_event (en_t event_id);
void ITC_write_all (int out_prod);
int OB_buffers_list (Buf_regist **buffers, int **n_bufs);
int OB_get_buffer_number (int prod_code);
int OB_get_elev_cnt (int olind);
void OB_get_id_from_name (char *dataname, int *data_id);
int OB_get_outbuf (int *mem, int *dattyp, int *bufsiz, int *bufptr, int *opstat);
Prod_header *OB_hd_info ();
int OB_hd_info_set ();
void OB_initialize (Orpgtat_entry_t *task_table);
int OB_out_list (Out_data_type **olist);
void OB_reg_outputs (int *status);
int OB_rel_all_outbufs (int *datdis);
int OB_rel_outbuf (int *bufptr, int *datdis, ...);
int OB_release_output_buffer (int ind, int olind, int disposition);
void OB_report_cpu_stats (char *task_name, unsigned int vol_scan_num, int vol_aborted, unsigned int expected_vol_dur);
void OB_set_prod_hdr_info (Base_data_header *hd, char *phd, int new_vol);
int OB_tag_outbuf_wreq (int *bufptr, short *user_array);
int OB_vol_number ();
void PRCPPROD_CD07_UPDT (int ITC_ID, int ACCESS);  /* prcpprod */
int PRQ_check_data (int *datatype, short *user_array, int *status);
Prod_request *PRQ_check_for_replay_requests (int *num_reqs);
int PRQ_check_req (int *datatype, short *user_array, int *status);
Prod_request *PRQ_get_prod_request (int data_type, int *n_requests);
void PRQ_initialize ();
Prod_request *PRQ_replay_product_requests (Prod_request *req, int length, int *num_req);
int PRQ_update_requests (int new_vol, int elev_ind, int time);
time_t PS_convert_to_unix_time (short date, short time_ms, short time_ls);
int PS_get_current_elev_index (int *index);
int PS_get_elev_angle (int *vcpnum, int *rpg_elev_ind, int *elev_angle, int *found);
time_t PS_get_volume_time (Base_data_header *bhd);
int PS_in_message_mode ();
void PS_message (char *format, ...);
void PS_message_mode ();
void PS_register_bd_hd (Base_data_header *bd_hd);
void PS_register_prod_hdr (Prod_header *phd);
void PS_task_abort (char *format, ...);
void Parse_args ();
int RPG_DEAU_callback_fx (int lb_fd, LB_id_t msgid, int msglen, char *gp_name);
void RPG_NINT (float *value, int *nearest_integer);
int RPG_UN_register (int *data_id, LB_id_t *msg_id, void (*service_routine)());
int RPG_abort_dataname_processing (char *dataname, int *reason);
int RPG_abort_datatype_processing (int *datatype, int *reason);
int RPG_abort_processing (int *reason);
int RPG_abort_request (void *request, int *reason);
void RPG_abort_task ();
int RPG_ade_get_number_of_values (char *alg_name, char *value_id);
int RPG_ade_get_string_values (char *alg_name, char *value_id, char **values);
int RPG_ade_get_values (char *alg_name, char *value_id, double *values);
int RPG_cleanup_and_abort (int *reason);
int RPG_clear_msg (int *size, char *message, int *status);
void RPG_compress_product (void *bufptr, int *method, int *status);
void RPG_data_access_group (int *group, int *data_id, int *msg_id, void *msg, int *msg_size, int *status);
void RPG_data_access_read (int *data_id, void *buf, int *buflen, int *msg_id, int *status);
void RPG_data_access_update (int *group, int *status);
void RPG_data_access_write (int *data_id, void *msg, int *length, int *msg_id, int *status);
void RPG_decompress_product (void *bufptr, char **out_bufptr, int *size, int *status);
int RPG_get_abort_reason (int *reason);
void RPG_get_buffer_elev_index (int *bufptr, int *elev_index);
void RPG_get_buffer_vcp_num (int *bufptr, int *vcp_num);
void RPG_get_buffer_vol_num (int *bufptr, int *vol_num);
void RPG_get_buffer_vol_seq_num (int *bufptr, unsigned int *ui_vol_num);
void RPG_get_code_from_id (int *prod_id, int *pcode);
void RPG_get_code_from_name (char *data_name, int *pcode);
int RPG_get_current_target_elev (int *elev);
void RPG_get_customizing_info (int *elev_index, short *user_array[][10], int *num_requests, int *status);
int RPG_get_inbuf_len (int *bufptr, int *len);
int RPG_get_product_float (void *loc, void *value);
int RPG_get_product_int (void *loc, void *value);
int RPG_get_request (int elev_index, int buf_type, int p_code, int *index, short *uarray);
void RPG_hari_kiri ();
int RPG_in_data (int *data_type, int *flags);
int RPG_in_opt (int *data_type, int *block_time);
int RPG_in_opt_by_name (char *data_name, int *block_time);
void RPG_init_log_services ();
int RPG_init_log_services_c ();
int RPG_is_adapt_block_registered (int *block_id, int *status);
void RPG_is_buffer_from_last_elev (int *bufptr, int *elev_index, int *last_elev_index);
int RPG_itc_callback (int *itc_id, int (*func)());
int RPG_itc_in (int *itc_id, void *first, void *last, int *sync_prd, ...);
int RPG_itc_out (int *itc_id, void *first, void *last, int *sync_prd, ...);
int RPG_itc_read (int *itc_id, int *status);
int RPG_itc_write (int *itc_id, int *status);
int RPG_monitor_input_buffer_load (int *data_id);
int RPG_out_data (int *data_type, int *timing, int *product_code);
int RPG_out_data_by_name_wevent (char *data_name, EN_id_t *event_id, int *product_id);
int RPG_out_data_wevent (int *data_type, int *timing, EN_id_t *event_id, int *product_code);
void RPG_prod_desc_block (void *ptr, int *prod_id, int *vol_scan_num, int *status);
void RPG_prod_hdr (void *ptr, int *prod_id, int *length, int *status);
int RPG_product_replay_request_response (int pid, int reason, short *dep_params);
int RPG_read_adapt_block (int *block_id, int *status);
int RPG_read_ade (fint *callback_id, fint *status);
void RPG_read_scan_summary ();
void RPG_read_volume_status ();
int RPG_reg_adpt (int *id, char *buf, int *timing, ...);
int RPG_reg_for_external_event (int *event_code, void (*service_routine)(), int *queued_parameter);
int RPG_reg_for_internal_event (int *event_code, void (*service_routine)(), int *queued_parameter);
void RPG_reg_inputs ();
void RPG_reg_io ();
int RPG_reg_moments (int *moments);
void RPG_reg_outputs ();
void RPG_reg_scan_summary ();
void RPG_reg_timer (int *parameter, void (*callback)());
void RPG_reg_volume_status ();
time_t RPG_scan_summary_last_updated ();
int RPG_send_msg (char *message);
void RPG_set_dep_params (void *ptr, short *params);
int RPG_set_mssw_to_uint (void *loc, unsigned int *value);
int RPG_set_product_float (void *loc, void *value);
int RPG_set_product_int (void *loc, void *value);
int RPG_set_veldeal_bit (short *flag_value);
void RPG_stand_alone_prod (void *ptr, char *string, int *length, int *status);
void RPG_task_init (int *what_based);
int RPG_task_init_c (int *what_based);
int RPG_update_all_ade ();
time_t RPG_volume_status_last_updated ();
int RPG_wait_act (int *wait_for);
int RPG_wait_for_any_data (int *wait_for, int *status);
void RPG_wait_for_event ();
int RPG_wait_for_task (char *task_name);
void RPG_what_moments (Base_data_header *hdr, int *refflag, int *velflag, int *widflag);
void *SS_get_summary_data ();
void SS_initialize ();
void SS_read_scan_summary ();
int SS_register ();
int SS_send_summary_array (int *summary);
void SS_update_summary (Base_data_header *bd_hd);
void T41192__JULIAN (int I, int J, int K, int *JULIAN);
void T41193__CALDATE (int JULIAN, int *IYEAR, int *JMONTH, int *KDAY);
void T41194__GETIME (float YMDHMS, float REF, int *IMSEC, short *MODJUL);
void TS_reg_timer (int timer_id, void (*callback)());
void *VS_get_volume_status ();
void VS_initialize ();
void VS_read_volume_status ();
int VS_send_volume_status (char *vol_stat);
void VS_update_volume_status (Base_data_header *bd_hd);
int WA_check_data (int buffer_ind);
int WA_check_data_prod (int buffer_ind);
int WA_check_replay_elev_vol_availability (int data_store_id, int sub_type, int vol_seq_num, int elev_ind);
int WA_data_filtering (In_data_type *Inp_list, int buffer_ind);
int WA_get_next_avail_input ();
Replay_req_info_t *WA_get_query_info ();
void WA_initialize ();
int WA_radial_status ();
int WA_set_resume_time ();
int WA_wait_driving ();
int WA_wait_for_any_data (int *wait_for, int *status, int wait_for_data);
int WA_waiting_for_activation ();
void a3cm40 (int *parameter, int *count, int *flag, int *ier);
void a3cm41 (int *parameter, int *count, int *flag, int *ier);
int abl (int *value, int *list, int *status);
int atl (int *value, int *list, int *status);
int date (int *yr);
int deflst (int *list, int *size);
int iclock (int *sw, int *buf);
int ilbyte (int *i, short j[3000], int *k);
void ioerr (fint *pblk, fint *status);
int isbyte (int *i, short *j, int *k);
int itoc_os32 (int *value, int *num_chars, char string[12]);
void lokoff (short *i);
int lokon (short *i);
int lstfun (int *fun, int *value, int *list, int *status);
int orpg_mem_addr (int indexed_mem);
int os32btest (int *data, int *off_in);
int os32btests (int *data, short off_in);
int os32sbtest (short *data, int *off_in);
int os32sbtests (short *data, short off_in);
int queue (char *rcv_name, int *parm, int *status);
int rbl (int *value, int *list, int *status);
int register_adapt (int *id, char *buf);
int rtl (int *value, int *list, int *status);
void sndmsg (char *rcv_name, int *msg, int *status);
int t41194__gettime (double *comp, double *ref, int *ms, short *date);
void wait (int *delay, int *unit, int *status);
int wait_c (int *delay, int *unit, int *status);
