/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 18:32:44 $
 * $Id: cpuuse_monitor.c,v 1.9 2014/03/18 18:32:44 jeffs Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

#include <cpuuse.h> 

/* static Global variables.  */
static time_t Prev_monitor_time = 0;
static Cpu_stats_t *Curr_cpu_stats = NULL;
static Cpu_stats_t *Prev_cpu_stats = NULL;
static int *Compare_index = NULL;
static int Cpu_stats_size = 0;
static Orpgtat_dir_entry_t *Dir = NULL;

static Vol_stats_t *Vol_stats[MAX_NUM_TASKS];
static int Vol_stats_num_elevs = 0;

/* Global variables. */
Vol_stats_list_t *List = NULL;
int Vol_stats_list_num_vols = 0;


/* Function prototypes.  */
static int Binary_search_name( Cpu_stats_t *array, int size, char *task_name,
                               int instance, char *node_name );
static void Insertion_sort( Orpgtat_dir_entry_t *array, int size );
static void Add_pid_to_list( Cpu_stats_t *stats, int pid, int cpu );
static void Free_list( Proc_stats_t *list );

/* Public Functions are define below. */

/******************************************************************

   Description:
      Initialization routine for the CPU use module.

   Inputs:

   Outputs:

   Returns:
      Always returns 0 on success.  On failure, calls exit().

   Notes:
      Any number of failures cause CPU monitoring to fail.

******************************************************************/
int CPUUSE_initialize( ){

   int ret, index;

   /* Build the TAT directory. */
   if( (ret = CPUUSE_build_tat_directory()) < 0 ){

      fprintf( stderr, "Unable to build TAT directory\n" );
      exit(1);

   }

   /* Allocate array of Cpu_stats_t structures to hold CPU stats. */
   Prev_cpu_stats = (Cpu_stats_t *) calloc( (size_t) 1, 
                                    (size_t) Cpu_stats_size*sizeof(Cpu_stats_t) );
   if( Prev_cpu_stats == NULL ){

      fprintf( stderr, "calloc Failed for %d bytes\n",
               Cpu_stats_size*sizeof(Cpu_stats_t) );
      free( Dir );
      exit(1);

   }

   Curr_cpu_stats = (Cpu_stats_t *) calloc( (size_t) 1, 
                                    (size_t) Cpu_stats_size*sizeof(Cpu_stats_t) );
   if( Curr_cpu_stats == NULL ){

      fprintf( stderr, "calloc Failed for %d bytes\n",
               Cpu_stats_size*sizeof(Cpu_stats_t) );
      free( Prev_cpu_stats );
      free( Dir);
      exit(1);

   }

   /* Allocate Compare_index array. */
   Compare_index = (int *) calloc( (size_t) 1, (size_t)  Cpu_stats_size * sizeof(int) );
   if( Curr_cpu_stats == NULL ){

      fprintf( stderr, "calloc Failed for %d bytes\n",
               Cpu_stats_size*sizeof(int) );
      free( Prev_cpu_stats );
      free( Curr_cpu_stats );
      free( Dir);
      exit(1);

   }

   /* Sort the tat array ... needs to be sorted in increasing order of
      task name. */
   Insertion_sort( Dir, Cpu_stats_size );

   /* Process each tat entry and initialize the cpu stats arrays. */
   for( index = 0; index < Cpu_stats_size; index++ ) {

      strcpy( Curr_cpu_stats[index].task_name, Dir[index].task_name );
      strcpy( Prev_cpu_stats[index].task_name, Curr_cpu_stats[index].task_name );
      strcpy( Curr_cpu_stats[index].node_name, Node_name );
      strcpy( Prev_cpu_stats[index].node_name, Node_name );
      Prev_cpu_stats[index].stats.cpu = Curr_cpu_stats[index].stats.cpu = 0;
      Prev_cpu_stats[index].stats.pid = Curr_cpu_stats[index].stats.pid = -1;
      Prev_cpu_stats[index].stats.next = Curr_cpu_stats[index].stats.next = NULL;
      Compare_index[index] = -1;

   /* End of "for" loop. */
   } 

   /* Initialize the volume stats array. */
   for( index = 0; index < MAX_NUM_TASKS; index++ ){

      Vol_stats_t *task = NULL;
      int ind;

      task = (Vol_stats_t *) calloc( 1, sizeof(Vol_stats_t) );
      if( task == NULL ){

         fprintf( stderr, "calloc Failed for %d bytes\n", sizeof(Vol_stats_t) );
         for( ind = 0; ind < index; ind++ ){

            if( Vol_stats[ind] != NULL )
               free( Vol_stats[ind] );

         }

         exit(1);

      }

      Vol_stats[index] = task;       

      /* If this data structure represents a task, then .... */
      if( index < Cpu_stats_size ){

         int i;

         for( i = 0; i < MAX_ELEVATIONS; i++ )
            task->cpu[i] = -1;

         task->task_name[0] = 0;
         strcpy( task->task_name, Dir[index].task_name );

      }
      else{

         int i;

         if( index == Cpu_stats_size ){

            /* The third to last entry in "task" is for the total cpu. */
            strcpy( task->label.units, "ms" );
            task->task_name[0] = 0;
            strcpy( task->task_name, "Total CPU" );

         }
         else if( index == (Cpu_stats_size+1) ){

            /* The second to last entry in "elev" is for the scan duration. */
            strcpy( task->label.units, "ms" );
            task->task_name[0] = 0;
            strcpy( task->task_name, "Scan Duration" );

         }
         else if( index == (Cpu_stats_size+2) ){

            /* The last entry in "elev" is for the percent total cpu. */
            strcpy( task->label.units, "%" );
            task->task_name[0] = 0;
            strcpy( task->task_name, "% Total CPU" );

         }

         /* Initialize the CPU numbers. */
            for( i = 0; i < MAX_ELEVATIONS; i++ )
               task->cpu[i] = 0;

      }

   }

   Vol_stats_num_elevs = 0;
   Vol_stats_list_num_vols = 0;
 
   /* Do not need the tat anymore. */
   if( Dir != NULL ){

      free( Dir );
      Dir = NULL;

   }

   /* Return success. */
   return(0);

/* End of CPUUSE_initialize() */
}

/******************************************************************
   Description:
      Saves the data in Vol_stats_t structure into a Vol_stats_list_t
      structure.

   Inputs:
      vol_hdr - pointer to volume scan header data corresponding
                to data in Vol_stats_t structure.

   Outputs:

   Returns:
      Assuming no memory allocation errors, always returns 0.

   Notes:

******************************************************************/
int CPUUSE_save_vol_stats( Vol_hdr_t *vol_hdr ){

   int i;

   Vol_stats_list_t *list = NULL;

   if( List == NULL ){

      List = calloc( 1, sizeof(Vol_stats_list_t));
      if( List == NULL ){

         fprintf( stderr, "calloc Failed for %d Bytes\n",
                  sizeof(Vol_stats_list_t) );
         exit(1);

      }

      list = List;

   }
   else{

      Vol_stats_list_t *next = List->next;
      Vol_stats_list_t *curr = List;

      while( next != NULL ){

         curr = next;
         next = curr->next;

      }
          
      next = calloc( 1, sizeof(Vol_stats_list_t));
      if( next == NULL ){

         fprintf( stderr, "calloc Failed for %d Bytes\n",
                  sizeof(Vol_stats_list_t) );
         exit(1);

      }

      curr->next = next;
      list = next;

   }
      
   /* Save the elevation data. */
   for( i = 0; i < MAX_NUM_TASKS; i++ ){

      list->vol_stats[i] = (Vol_stats_t *) calloc( 1, sizeof(Vol_stats_t) );

      if( list->vol_stats[i] == NULL ){

         fprintf( stderr, "calloc Failed for %d bytes\n", sizeof(Vol_stats_t) );
         exit(1);

      }

      memcpy( list->vol_stats[i], Vol_stats[i], sizeof(Vol_stats_t) );

   }

   /* Save the number of elevations for this volume scan, the volume scan date/time,
      the VCP number, and the volume scan duration. */
   list->num_elevs = Vol_stats_num_elevs;
   memcpy( &list->vol_hdr.vol.dt, &vol_hdr->vol.dt, sizeof(Date_time_t) );
   list->vol_hdr.vol.vcp_number = vol_hdr->vol.vcp_number;
   list->vol_hdr.vcp_duration = vol_hdr->vcp_duration;

   /* Increment the number of volume scans. */
   Vol_stats_list_num_vols++;

   return(0);

}

/******************************************************************
   Description:
      Initializes a Vol_stats_t data structure for subsequent use.

   Inputs:

   Outputs:

   Returns:
      Always returns 0.

   Notes:

******************************************************************/
int CPUUSE_init_vol_stats(){

   int index, ind;
   Vol_stats_t *task = NULL;

   /* Initialize the volume stats array. */
   for( index = 0; index < MAX_NUM_TASKS; index++ ){

      task = Vol_stats[index];       

      for( ind = 0; ind < MAX_ELEVATIONS; ind++ ){

         if( index < Cpu_stats_size )
             task->cpu[ind] = -1;
    
         else if( index >= Cpu_stats_size )
             task->cpu[ind] = 0;

      }

   }

   Vol_stats_num_elevs = 0;

   return 0;

/* End of CPUUSE_init_vol_stats(). */
}

/******************************************************************

   Description:
      Reads and reports RPG process CPU utilization.

   Inputs:

   Outputs:

   Return:	
      0 on success or -1 on failure.
	
   Notes:

******************************************************************/
int CPUUSE_compute_cpu_utilization( char *buf, time_t monitor_time,
                                    int elevation_cut, int len ){

    int ind, index, num_compare;
    unsigned int cpu_diff, total_cpu;
    time_t time_diff;
    double cpu_diff_percent = 0.0;
    char *cpt;

    /* Initialize the current cpu stats list. */
    for( index = 0; index < Cpu_stats_size; index++ ){

       (Curr_cpu_stats + index)->stats.cpu = 0;
       (Curr_cpu_stats + index)->stats.pid = -1;
       if( (Curr_cpu_stats + index)->stats.next != NULL )
          Free_list( (Curr_cpu_stats + index)->stats.next );

       (Curr_cpu_stats + index)->stats.next = NULL;

    /* End of "for" loop. */
    }
   
    /* Do For All processes being reported. */
    cpt = buf;
    num_compare = 0;
    while (1){

       Task_rec_t *ps;

       ps = (Task_rec_t *)cpt;
       if( (cpt - buf + sizeof(Task_rec_t) > len) )
	    break;

        /* Find the entry of task in current stats list. */
        index = Binary_search_name( Curr_cpu_stats, Cpu_stats_size,
                                    ps->task_name, ps->instance, ps->node_name );


        /* Binary search found match on task name. */
        if( index >= 0 ){
        
           /* If this is the first encountered instance of this process,
              set the pid and total cpu comsumed so far. */
           if( (Curr_cpu_stats + index)->stats.pid < 0 ){

	      (Curr_cpu_stats + index)->stats.cpu = ps->cpu;
	      (Curr_cpu_stats + index)->stats.pid = ps->pid;

              /* Save the index for later comparison. */
              *(Compare_index + num_compare) = index;

              /* Increment the number of processes to compare. */
              num_compare++;

           }
           else{

              Proc_stats_t *curr = &(Curr_cpu_stats + index)->stats;

              /* To account for multiple instances of this process, we
                 add to an entry for this pid. */
              Add_pid_to_list( (Curr_cpu_stats + index), ps->pid, ps->cpu );

              while(1){

                 curr = curr->next;
                 if( curr == NULL )
                    break;

              }

           }

        }
        else if( index == -1 )
           fprintf( stderr, "Task %s Not In Task Table\n", ps->task_name );

	cpt += sizeof(Task_rec_t);

    /* end of "while" loop. */
    }

    /* First time monitoring initialization. */
    if( Prev_monitor_time == 0 ){

       for( index = 0; index < Cpu_stats_size; index++ ){

          (Prev_cpu_stats + index)->stats.cpu = (Curr_cpu_stats + index)->stats.cpu;
          (Prev_cpu_stats + index)->stats.pid = (Curr_cpu_stats + index)->stats.pid;
          if( (Curr_cpu_stats + index)->stats.next != NULL ){

             Proc_stats_t *curr = (Curr_cpu_stats + index)->stats.next;
             while( curr != NULL ){

                Add_pid_to_list( (Prev_cpu_stats + index), curr->pid,
                                  curr->cpu );
                curr = curr->next;

             /* End of "while" loop. */
             }

          }

       /* End of "for" loop. */
       }

       Prev_monitor_time = monitor_time;
 
    }
    else{

       /* Initialize the total cpu. */
       total_cpu = 0;

       /* Compute CPU statistics for this monitoring period. */
       for( index = 0; index < num_compare; index++ ){

          Proc_stats_t *curr;

          /* Only consider those processes which were active during
             this monitoring period. */
          ind = Compare_index[index];
          curr = &(Curr_cpu_stats + ind)->stats;

          /* Initialize the cpu difference.  */
          cpu_diff = 0;

          while(1){

             /* Check for match on pid. */
             if( curr->pid == (Prev_cpu_stats + ind)->stats.pid )
                cpu_diff += ( curr->cpu - (Prev_cpu_stats + ind)->stats.cpu );

             else{

                Proc_stats_t *prev = (Prev_cpu_stats + ind)->stats.next;
                int match = 0;

                /* Check multiple instances for match on pid. */
                while( prev != NULL ){

                   if( curr->pid == prev->pid ){

                      /* Match found. */
                      match = 1;
                      break;

                   }

                   prev = prev->next;

                /* End "while" loop. */
                }

                if( match )
                   cpu_diff += (curr->cpu - prev->cpu);

                else
                   cpu_diff += curr->cpu;

             }

             /* Do for all instances of this task found this monitoring period. */
             curr = curr->next;
             if( curr == NULL )
                break;

          /* End of "while" loop. */
          }

          /* Add this cpu difference to the appropriate elevation. */
          if( Vol_stats[ind]->cpu[elevation_cut-1] <= 0 ) 
             Vol_stats[ind]->cpu[elevation_cut-1] = cpu_diff;

          else
             Vol_stats[ind]->cpu[elevation_cut-1] += cpu_diff;

          /* Add difference to total. */
          total_cpu += cpu_diff;

       /* End of "for" loop. */
       }

       /* Set the number of elevation cuts this VCP. */
       Vol_stats_num_elevs = elevation_cut;

       /* Set the total CPU for this elevation. */
       Vol_stats[Cpu_stats_size]->cpu[elevation_cut-1] = total_cpu;

       /* CPU is reported in millisecs and time is in secs so
          need to divide by 1000. */
       cpu_diff_percent = (double) total_cpu / 1000.0;
       time_diff = monitor_time - Prev_monitor_time;

       /* Set the scan duration, in millisecs. */
       Vol_stats[Cpu_stats_size+1]->cpu[elevation_cut-1] = time_diff * 1000;

       /* Set the percent total CPU for this elevation.  Scale by 10.0 */
       Vol_stats[Cpu_stats_size+2]->cpu[elevation_cut-1] = 
                                  (int) ((cpu_diff_percent / time_diff) * 1000);

       /* Prepare for next monitoring cycle. */
       Prev_monitor_time = monitor_time;
       for( index = 0; index < Cpu_stats_size; index++ ){

          (Prev_cpu_stats + index)->stats.cpu = (Curr_cpu_stats + index)->stats.cpu;
          (Prev_cpu_stats + index)->stats.pid = (Curr_cpu_stats + index)->stats.pid;

          if( (Prev_cpu_stats + index)->stats.next != NULL ){

             Free_list( (Prev_cpu_stats + index)->stats.next );
             (Prev_cpu_stats + index)->stats.next = NULL;

          }

          if( (Curr_cpu_stats + index)->stats.next != NULL ){

             Proc_stats_t *next = (Curr_cpu_stats + index)->stats.next;
             while( next != NULL ){

                Add_pid_to_list( (Prev_cpu_stats + index), next->pid, next->cpu );
                next = next->next;

             /* End of "while" loop. */
             }

          }

       /* End of "for" loop. */
       }

    }

    /* Return to application. */
    return (0);

/* End of CPUUSE_compute_cpu_utilization() */
}

/*******************************************************************
   Description:
      Writes the cpu utiliztion for the current volume scan to 
      standard output device.   The output is in formatted for 
      ease of reading and interpretation.

   Inputs:
      vol_stats_list - pointer to Vol_stats_list_t structure 
                       containing the volume statistics.

   Outputs:

   Returns:
      Always returns 0.

   Notes:

*******************************************************************/
int CPUUSE_write_cpu_utilization( Vol_stats_list_t *vol_stats_list ){

   int i, j, cut; 
   float task_total_cpu, total_cpu;
   Vol_stats_t *task;
   int vcp_duration = vol_stats_list->vol_hdr.vcp_duration;

   total_cpu = 0.0;

   fprintf( stdout, "         Task Name  " );
   for( j = 0; j <= (vol_stats_list->num_elevs+1); j++ ){

      if( j < vol_stats_list->num_elevs ){

         cut = j+1;
         fprintf( stdout, "  %6d", cut );

      }
      else if( j == vol_stats_list->num_elevs )
         fprintf( stdout, "    Total" );

      else 
         fprintf( stdout, "    Percent Total\n\n" );

   }
   
   /* Do For All tasks in task table ..... */
   for( j = 0; j <= (Cpu_stats_size+STATS_OVERHEAD); j++ ){ 

      /* For convenience .... */
      task = vol_stats_list->vol_stats[j];

      /* Put in a line feed after the last operational task. */
      if( j == Cpu_stats_size )
         fprintf( stdout, "\n" );

      if( j < Cpu_stats_size )
         fprintf( stdout, "%20s", task->task_name );

      else{

         char label[21];

         label[0] = '\0';
         sprintf( label, "%15s (%2s)", task->task_name, task->label.units );
         fprintf( stdout, "%20s", label );

      }
 
      task_total_cpu = -1.0f;
      for( i = 0; i < vol_stats_list->num_elevs; i++ ){

         if( j <= (Cpu_stats_size+1) )
            fprintf( stdout, "  %6d", task->cpu[i] );

         else
            fprintf( stdout, "  %6.1f", ((float) task->cpu[i])/10.0 );

         if( task->cpu[i] >= 0 ){

            if( task_total_cpu < 0.0 )
               task_total_cpu = (float) task->cpu[i];
    
            else{

               if( j <= (Cpu_stats_size+1) )
                  task_total_cpu += (float) task->cpu[i];

               else
                  task_total_cpu += ((float) task->cpu[i])/10.0;

            }

         }

      }

      /* Write out task total CPU and task total CPU as a percent of volume scan. */
      if( j < Cpu_stats_size ){

         /* Time is in milliseconds, duration in seconds, but we want answer in %. */
         float percent_total = task_total_cpu / (vcp_duration*10.0);

         if( percent_total < 0.0 )
            percent_total = -1.0;

         fprintf( stdout, "   %6d", (int) round(task_total_cpu) );
         fprintf( stdout, "   %6.1f", percent_total );

      }
      else if( j <= (Cpu_stats_size+1) )
         fprintf( stdout, "   %6d", (int) round(task_total_cpu) );

      /* Add the task total cpu to the cumulative total cpu. */
      if( (task_total_cpu >= 0) && (j < Cpu_stats_size) )
         total_cpu += task_total_cpu;

      /* Write out the volume total CPU utilization. */
      if( j == (Cpu_stats_size+STATS_OVERHEAD) ){

         float percent_total = total_cpu / (vcp_duration*10.0);

         fprintf( stdout, "   %6.1f", percent_total );

      }
         
      fprintf( stdout, "\n" );

   }

   /* Put some space in. */
   fprintf( stdout, "\n\n\n\n" );

   return 0;
}

/*******************************************************************
   Description:

   Inputs:

   Outputs:

   Returns:

   Notes:

*******************************************************************/
int CPUUSE_sum_and_avg( Vol_stats_list_t **array, 
                        Vol_stats_list_t **avg,
                        int start_vol, int end_vol ){

   Vol_stats_t *in_task = NULL;
   Vol_stats_t *out_task = NULL;

   Vol_stats_list_t *curr = NULL, *local_avg = NULL;
   int vol_total_cpu, elev_total_cpu[MAX_ELEVATIONS];
   int vcp_duration, vcp, num_elevs, num_volumes, i, j, k;

   /* First we must validate the input statistics.   They must all
      come from the same VCP. */
   k = start_vol;
   vcp = -1;

   /* Do for each volume ...... */
   while( k <= end_vol ){

      /* Set pointer to the task in the input statistics. */
      curr = array[k-1];
      assert( curr != NULL );

      if( vcp < 0 )
         vcp = curr->vol_hdr.vol.vcp_number;

      if( curr->vol_hdr.vol.vcp_number != vcp ){

         fprintf( stderr, "In order to Average Volumes, the VCPs MUST all be the same\n" );
         return(-1);

      }

      k++;

   }
   
   /* If memory not allocated for "avg", allocate it here. */
   if( *avg == NULL ){

      *avg = (Vol_stats_list_t *) calloc( 1, sizeof(Vol_stats_list_t));
      if( *avg == NULL ){

         fprintf( stderr, "calloc Failed for %d Bytes\n",
                  sizeof(Vol_stats_list_t));
         exit(1);

      }

      /* Allocate storage for each task entry ..... */
      for( i = 0; i < MAX_NUM_TASKS; i++ ){

         (*avg)->vol_stats[i] = (Vol_stats_t *) calloc( 1, sizeof(Vol_stats_t) );

         if( (*avg)->vol_stats[i] == NULL ){

            fprintf( stderr, "calloc Failed for %d bytes\n", sizeof(Vol_stats_t) );
            exit(1);

         }

         for( j = 0; j < MAX_ELEVATIONS; j++ ){

            (*avg)->vol_stats[i]->cpu[j] = -1;
            elev_total_cpu[j] = -1;

         }

      }

   }

   /* For convenience ... */
   local_avg = *avg;

   /* Set some "general" type information. */
   k = start_vol;
   num_volumes = 0;
   num_elevs = 0;
   vcp_duration = 0;

   /* Do for each volume ...... */
   while( k <= end_vol ){

      /* Set pointer to the task in the input statistics. */
      curr = array[k-1];
      assert( curr != NULL );

      num_elevs += curr->num_elevs;
      num_volumes++;
      vcp_duration += curr->vol_hdr.vcp_duration;

      k++;

   }
   
   if( num_volumes > 0 ){

      local_avg->num_elevs = num_elevs / num_volumes;
      local_avg->vol_hdr.vcp_duration = vcp_duration / num_volumes;

   }

   /* Do for each task ..... */
   for( j = 0; j <= (Cpu_stats_size+2); j++ ){ 

      /* Set pointer to task in the output statistics. */
      out_task = local_avg->vol_stats[j];

      /* Do for each elevation scan .... */
      for( i = 0; i < MAX_ELEVATIONS; i++ ){

         k = start_vol;
         num_volumes = 0;
         vol_total_cpu = -1;

         /* Do for each volume ...... */
         while( k <= end_vol ){

            /* Set pointer to the task in the input statistics. */
            curr = array[k-1];
            assert( curr != NULL );

            in_task = curr->vol_stats[j];

            /* Copy the task name from input statistics to output
               statistics. */
            memcpy( out_task->task_name, in_task->task_name, ORPG_TASKNAME_SIZ );
            memcpy( &out_task->label, &in_task->label, sizeof(linelabel_t) );

            /* Accumulate the CPU on a per elevation basis for this task and
               over all volume scans to report. */
            if( in_task->cpu[i] >= 0 ){

               num_volumes++;
               if( vol_total_cpu < 0 )
                  vol_total_cpu = in_task->cpu[i];
    
               else
                  vol_total_cpu += in_task->cpu[i];

            }

            k++;

         }

         /* Take the average for the elevation/task combination over all
            volumes scans in which CPU was reported. */
         if( num_volumes > 0 )
            out_task->cpu[i] = vol_total_cpu / num_volumes;

         else
            out_task->cpu[i] = -1;
         
         /* Accumulate the elevation total CPU. */
         if( elev_total_cpu[i] < 0 )
            elev_total_cpu[i] = out_task->cpu[i];

         else
            elev_total_cpu[i] += out_task->cpu[i];

      }

   }

   return 0;
}

/***********************************************************************

   Description:
      Builds TAT directory based on ASCII TAT.

   Inputs:

   Outputs:

   Returns:
      Always returns 0.  On error, cpuuse exits.

   Notes:

***********************************************************************/
int CPUUSE_build_tat_directory(){

   Orpgtat_mem_ary_t mem_ary ;
   int byte_cnt, index, retval;

   /* Read Task Attribute Table entries from the Task Table 
      Configuration File. */
   mem_ary.size_bytes = 0 ;
   mem_ary.ptr = NULL ;

   retval = ORPGTAT_read_ascii_tbl( &mem_ary, Ttcf_fname ) ;
   if( retval != 0 ){

       fprintf( stderr, "ORPGTAT_read_ascii_tbl() returned %d",
                retval) ;
       exit(1);

   }

   /* Allocate storage to hold a TAT directory. */
   Dir = (Orpgtat_dir_entry_t *) calloc( 1, 
                     sizeof(Orpgtat_dir_entry_t)*MAX_NUM_TASKS );
   if( Dir == NULL ){

      fprintf( stderr, "calloc Failed for %d Bytes\n",
               sizeof(Orpgtat_dir_entry_t)*MAX_NUM_TASKS );
      exit(1);

   }

   byte_cnt = 0;
   index = 0;
   while (byte_cnt < mem_ary.size_bytes){

      int instance = -1;
      Orpgtat_entry_t *entry_p = NULL;

      entry_p = (Orpgtat_entry_t *)
                         ((char *) mem_ary.ptr + byte_cnt) ;

      while(1){

         if( strlen( entry_p->task_name ) >= ORPG_TASKNAME_SIZ )
            strncpy( Dir[index].task_name, entry_p->task_name, (size_t) ORPG_TASKNAME_SIZ-1 );
         else
            strcat( Dir[index].task_name, entry_p->task_name );

         /* Increment the instance.  The instance number is a non-negative 
            number. */
         instance++;

         /* Append the instance number if multiple instance task. */
         if( entry_p->maxn_inst > 1 ){

            char inst[4];

            memset( inst, 0, 4 );
            if( instance <= 9 )
               sprintf( inst, ".%1d", instance );

            else
               sprintf( inst, ".%2d", instance );
            strcat( Dir[index].task_name, inst );

         }

         /* Prepare for next TAT entry. */
         index++;

         /* No more instances. */
         if( (instance+1) >= entry_p->maxn_inst )
            break;

      }

      /* Go to the next TAT entry. */
      byte_cnt += entry_p->entry_size ;


   }

   Cpu_stats_size = index;

   free(mem_ary.ptr) ;
   return 0;

}

/*******************************************************************
   Description:
      This function performs a binary search of an array of 
      Cpu_stats_t entries based on task name.  If a match found,
      returns index into array.
  
   Inputs:
      array - pointer to array to search.
      size - size of array to search.
      name - the task name to find.
      instance - task instance number.
      node_name - node name associated with stats.
  
   Returns:
      Returns index to entry in array, or -1 is entry not found 
      in task table, or -2 if entry not running on the node of
      interest.

   Notes:

******************************************************************/
static int Binary_search_name( Cpu_stats_t *array, int size, char *name,
                               int instance, char *node_name ){

   int top, middle, bottom;
   static char task_name[ORPG_TASKNAME_SIZ];
   
   /* Make local copy of task name. */
   memset( task_name, 0, ORPG_TASKNAME_SIZ );
   strcat( task_name, name );

   /* If multiple instance task, append the instance number. */
   if( instance >= 0 ){

      char inst[4];

      memset( inst, 0, 4 );
      if( instance <= 9 )
         sprintf( inst, ".%1d", instance );

      else
         sprintf( inst, ".%2d", instance );
      strcat( task_name, inst ); 

   }

   /* Set the list top and bottom bounds. */
   top = size - 1;
   if( top == -1 )
      return (-1);
  
   bottom = 0;
   
   /* Do Until task_name found or task_name not in list. */
   while( top > bottom ){
  
      middle = (top + bottom)/2;
      if( strcmp( task_name, (array + middle)->task_name) > 0 )
         bottom = middle + 1;
  
      else if( strcmp( task_name, (array + middle)->task_name ) == 0 ){

         if( (((array + middle)->node_name != NULL) 
                              && 
              (strcmp( (array + middle)->node_name, node_name ) == 0 ))
                        ||
             ((node_name[0] == '\0') || ((array + middle)->node_name[0] == '\0')) )
            return ( middle );

         else
            return (-2);
  
      }
      else
         top = middle;
  
   /* End of "while" loop. */
   }
   
   /* Modify the task status. */
   if( strcmp( (array + top)->task_name, task_name ) == 0 ){

         if( (((array + top)->node_name != NULL) 
                              && 
              (strcmp( (array + top)->node_name, node_name ) == 0 ))
                        ||
             ((node_name[0] == '\0') || ((array + top)->node_name[0] == '\0')) )
         return ( top );

      else
         return (-2);
   
   }
   else
      return (-1);
   
/* End of Binary_search_name( ) */
}              

/*******************************************************************
   Description:
      This function sorts the tat entries into increasing order
      of task_name.  This is required for the binary search to work
      correctly.
  
   Inputs:
      array - pointer to array to sort.
      size - size of array to sort.
  
   Returns:
      Returns void.

   Notes:
      Insertion sort was choosen since the input array should be
       very nearly sorted. 

******************************************************************/
static void Insertion_sort( Orpgtat_dir_entry_t *array, int size ){

   int index, place;
   Orpgtat_dir_entry_t current;
   
   /* Sort the array. */
   for( index = 1; index < size; index++ ){
  
      if( strcmp( (array + index)->task_name, (array + index - 1)->task_name) < 0 ){

         current = *(array+index); 
         for( place = index-1; place >= 0; place-- ){

            *(array + place + 1) = *(array + place);
            if( (place == 0) 
                      ||
                (strcmp( (array + place - 1)->task_name, current.task_name)) <= 0 )
               break; 

         }

         *(array + place) = current;

      }
  
   /* End of "for" loop. */
   }
   
/* End of Insertion_sort( ) */
}              

/*******************************************************************

   Description:
      The function saves the cpu for a multiple instance of a process.

   Inputs:
      task_entry - pointer to Cpu_stats_t structure.
      pid - Process ID.
      cpu - accumulated CPU time.

   Outputs:

   Returns:
      void.

   Notes:

*******************************************************************/
static void Add_pid_to_list( Cpu_stats_t *task_entry, int pid, int cpu ){

   Proc_stats_t *stats = &task_entry->stats;

   while(1){

      if( stats->next == NULL ){

         stats->next = (Proc_stats_t *) malloc( (size_t) sizeof(Proc_stats_t) );
         if( stats->next == NULL ){

            fprintf( stderr, "malloc Failed for %d Bytes\n",
                     sizeof( Proc_stats_t ) );

            exit(1);

         }

         stats->next->pid = pid;
         stats->next->cpu = cpu;
         stats->next->next = NULL;
  
         break;
      }

      stats = stats->next;

   }
      
/* End of Add_pid_to_list() */
}

/**********************************************************************

   Description:
      Frees list of cpu statistic for multiple instance processes.

   Input:
      list - list of Proc_stat_t structures.

   Output:

   Returns:
      void.

   Notes:

**********************************************************************/
static void Free_list( Proc_stats_t *list ){

   Proc_stats_t *curr_pid = list;
   Proc_stats_t *next_pid = curr_pid->next;

   while( curr_pid != NULL ){

      free( curr_pid );
      curr_pid = next_pid;
      if( curr_pid != NULL )
          next_pid = curr_pid->next;

   /* End of "while" loop. */
   }

/* End of Free_list() */
}

