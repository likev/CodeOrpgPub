/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/07/20 18:07:04 $
 * $Id: ps_monitor_cpu.c,v 1.15 2009/07/20 18:07:04 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <orpg.h> 
#include <infr.h> 

#include <ps_def.h>
#include <mrpg.h>

#define PS_MONITOR_CPU
#include <ps_globals.h>
#undef PS_MONITOR_CPU

typedef struct proc_stats_t{

   int pid;
   int cpu;
   struct proc_stats_t *next;

} Proc_stats_t;
   
/* Structure definition for holding CPU statistics. */
typedef struct cpu_stats_t {

   char task_name[ ORPG_TASKNAME_SIZ ];
   struct proc_stats_t stats;

} Cpu_stats_t;

/*
  static Global variables. 
*/
static time_t Prev_monitor_time = 0;
static Cpu_stats_t *Curr_cpu_stats = NULL;
static Cpu_stats_t *Prev_cpu_stats = NULL;
static int *Compare_index = NULL;
static int Cpu_stats_size = 0;

static double Cpu_overhead = 0.0;

/*
  Function prototypes. 
*/
static void Cpu_mon_alarm_callback( malrm_id_t alarmid );
static void Task_status_callback( int fd, LB_id_t msgid, int msg_info, 
                                  void *arg );
static int Binary_search( Cpu_stats_t *array, int size, char *task_name );
static void Insertion_sort( Orpgtat_dir_entry_t *array, int size );
static void Add_pid_to_list( Cpu_stats_t *stats, int pid, int cpu );
static void Free_list( Proc_stats_t *list );

/* Public Functions are define below. */
/******************************************************************

   Description: 
      The main function for periodic CPU monitoring.

   Inputs:

   Outputs:

   Returns:

   Notes:

******************************************************************/
int MCPU_monitor_cpu() {
   
   Psg_cpu_info_avail = 0;

   /* Send command to mrpg to report status. */
   if( ORPGMGR_send_command( MRPG_STATUS ) < 0 ){

      LE_send_msg( GL_INFO, "mrpg Status Request Failed\n" );
      return( PS_DEF_FAILED );

   }

   return( PS_DEF_SUCCESS );

/* End of MCPU_monitor_cpu() */
}

/******************************************************************

   Description:
      Initialization routine for the CPU monitoring module.

   Inputs:

   Outputs:

   Returns:

   Notes:
      Any number of failures cause CPU monitoring to be disabled.

******************************************************************/
void MCPU_initialize(){

   int ret, index;
   time_t start_time;
   Orpgtat_dir_entry_t *tat;

   Psg_cpu_info_avail = 0;
   Psg_cpu_monitor_alarm_expired = 0;

   if( Psg_cpu_monitoring_rate == 0 ){

      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      return;

   }

   Cpu_overhead = 100.0 / (100.0 - (double) Psg_cpu_overhead);

   if( Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL ){

      LE_send_msg( GL_INFO, "CPU Monitoring Rate: %d Seconds\n",
                   Psg_cpu_monitoring_rate );
      LE_send_msg( GL_INFO, "CPU Overhead:        %d Percent\n",
                   Psg_cpu_overhead );

   }

   /* Register CPU monitoring alarm. */
   ret = MALRM_register( (malrm_id_t) MALRM_CPU_MONITOR, 
                         Cpu_mon_alarm_callback );
   if( ret < 0 ){

      LE_send_msg( GL_INFO, "Unable to Set CPU Monitoring Alarm\n" );
      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      Psg_cpu_monitoring_rate = 0;
      return; 
 
   }

   /* Register for notification on task status. */
   ret = ORPGDA_UN_register( ORPGDAT_TASK_STATUS, MRPG_PS_MSGID, 
                             Task_status_callback );
   if( ret < 0 ){

      LE_send_msg( GL_INFO, "Unable to Register For Task Status Updates.\n" );
      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      Psg_cpu_monitoring_rate = 0;
      return;

   }

   /* Set the CPU monitoring alarm. */
   start_time = time(NULL) + Psg_cpu_monitoring_rate;
   ret = MALRM_set( MALRM_CPU_MONITOR, start_time, 
                    Psg_cpu_monitoring_rate );
   if( ret < 0 ){

      LE_send_msg( GL_INFO, "MALRM_set For MALRM_CPU_MONITOR Failed (%d)\n",
                   ret );
      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      Psg_cpu_monitoring_rate = 0;
      return;

   }
   
   /* Read the Task Table directory. */
   if( (Cpu_stats_size = ORPGTAT_get_directory( &tat )) < 0 ){

      LE_send_msg( GL_INFO, "Unable to get TAT directory\n" );
      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      Psg_cpu_monitoring_rate = 0;
      return;

   }

   LE_send_msg( GL_INFO, "There are %d entries in the TAT\n", Cpu_stats_size ); 

   /* Allocate array of Cpu_stats_t structures to hold CPU stats. */
   Prev_cpu_stats = (Cpu_stats_t *) calloc( (size_t) 1, 
                                    (size_t) Cpu_stats_size*sizeof(Cpu_stats_t) );
   if( Prev_cpu_stats == NULL ){

      LE_send_msg( GL_INFO, "calloc Failed for %d bytes\n",
                   Cpu_stats_size*sizeof(Cpu_stats_t) );
      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      Psg_cpu_monitoring_rate = 0;
      free( tat );
      return;

   }

   Curr_cpu_stats = (Cpu_stats_t *) calloc( (size_t) 1, 
                                    (size_t) Cpu_stats_size*sizeof(Cpu_stats_t) );
   if( Curr_cpu_stats == NULL ){

      LE_send_msg( GL_INFO, "calloc Failed for %d bytes\n",
                   Cpu_stats_size*sizeof(Cpu_stats_t) );
      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      Psg_cpu_monitoring_rate = 0;
      free( Prev_cpu_stats );
      free( tat);
      return;

   }

   /* Allocate Compare_index array. */
   Compare_index = (int *) calloc( (size_t) 1, (size_t)  Cpu_stats_size * sizeof(int) );
   if( Curr_cpu_stats == NULL ){

      LE_send_msg( GL_INFO, "calloc Failed for %d bytes\n",
                   Cpu_stats_size*sizeof(int) );
      LE_send_msg( GL_INFO, "CPU Monitoring DISABLED\n" );
      Psg_cpu_monitoring_rate = 0;
      free( Prev_cpu_stats );
      free( Curr_cpu_stats );
      free( tat);
      return;

   }

   /* Sort the tat array ... needs to be sorted in increasing order of
      task id. */
   Insertion_sort( tat, Cpu_stats_size );

   /* Process each tat entry and initialize the cpu stats arrays. */
   for( index = 0; index < Cpu_stats_size; index++ ) {

      strcpy( (char *) &(Curr_cpu_stats[index].task_name[0]), 
              (char *) &(tat[index].task_name[0]) );
      strcpy( (char *) &(Prev_cpu_stats[index].task_name[0]), 
              (char *) &(Curr_cpu_stats[index].task_name[0]) );
      Prev_cpu_stats[index].stats.cpu = Curr_cpu_stats[index].stats.cpu = 0;
      Prev_cpu_stats[index].stats.pid = Curr_cpu_stats[index].stats.pid = -1;
      Prev_cpu_stats[index].stats.next = Curr_cpu_stats[index].stats.next = NULL;
      Compare_index[index] = -1;

   /* End of "for" loop. */
   } 

   /* Do not need the tat anymore. */
   free( tat );

/* End of MCPU_initialize() */
}

/******************************************************************

   Description:
      Reads and reports RPG process CPU utilization.

   Inputs:

   Outputs:

   Return:	
      PS_DEF_SUCCESS on success or PS_DEF_FAILED on failure.
	
   Notes:

******************************************************************/
int MCPU_report_cpu_utilization(){

    int len, cpu_percent, ind, index, num_compare;
    unsigned int total_cpu;
    double cpu_diff;
    time_t monitor_time, time_diff;
    char *buf, *cpt;

    len = ORPGDA_read (ORPGDAT_TASK_STATUS, 
			(char *)&buf, LB_ALLOC_BUF, MRPG_PS_MSGID);
    if (len < 0) {

	LE_send_msg( GL_INFO, 
                     "ORPGDA_read process status failed (ret %d)\n", len);
	return (PS_DEF_FAILED);

    }
    else if (len == 0){

	LE_send_msg( GL_INFO, "No operational process is running\n");
	return (PS_DEF_SUCCESS);

    }

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

       Mrpg_process_status_t *ps;

       ps = (Mrpg_process_status_t *)cpt;
       if( (cpt - buf + sizeof(Mrpg_process_status_t) > len)
                            ||
            (cpt - buf + ps->size > len) )
	    break;

       if( ps->name_off == 0 ){

          LE_send_msg( GL_INFO, "CPU Stats Unavailable\n" );
          return(PS_DEF_FAILED);

       }  

        /* Find the entry of task in current stats list. */ 
        index = Binary_search( Curr_cpu_stats, Cpu_stats_size,
                               (cpt + ps->name_off) );

        /* Binary search found match on task ID. */
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
        else if (Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL )
           LE_send_msg( GL_INFO, "Task Not In Task Table\n" );

	cpt += ps->size;

    /* end of "while" loop. */
    }

    /* Free the task status information. */
    free( buf );

    /* Initialize the total cpu. */
    total_cpu = 0;

    /* Set the monitoring time. */
    monitor_time = time(NULL);

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

       /* Compute CPU statistics for this monitoring period. */
       for( index = 0; index < num_compare; index++ ){

          Proc_stats_t *curr;

          /* Only consider those processes which were active during
             this monitoring period. */
          ind = Compare_index[index];
          curr = &(Curr_cpu_stats + ind)->stats;

          while(1){

             /* Check for match on pid. */
             if( curr->pid == (Prev_cpu_stats + ind)->stats.pid )
                total_cpu += ( curr->cpu - (Prev_cpu_stats + ind)->stats.cpu );

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
                   total_cpu += (curr->cpu - prev->cpu);

                else
                   total_cpu += curr->cpu;

             }

             /* Do for all instances of this task found this monitoring period. */
             curr = curr->next;
             if( curr == NULL )
                break;

          /* End of "while" loop. */
          }

       /* End of "for" loop. */
       }

       /* CPU is reported in millisecs and time is in secs so
          need to divide by 1000. */
       cpu_diff = (double) total_cpu / 1000.0;
       time_diff = monitor_time - Prev_monitor_time;

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

       /* Report the CPU utilization. */
       if( time_diff != 0 ){

          /* CPU is reported in percentage.  Round to nearest integer. */
          cpu_percent = (int) (((cpu_diff / (double) time_diff) * 100.0 ) + 0.5 );

          /* Account for the overhead. */
          cpu_percent = (int) (cpu_percent * Cpu_overhead ); 

          /* Report the CPU utilization. */
          if( Psg_verbose_level >= PS_DEF_INFO_VERBOSE_LEVEL )
             LE_send_msg( GL_INFO, "CPU diff:  %lf, Time diff:  %d, CPU load: %d\n", 
                          cpu_diff, time_diff, cpu_percent );

          /* If we want to set the CPU load, we do it here!!
          if( (ret = ORPGLOAD_set_data( LOAD_SHED_CATEGORY_CPU,
                                        LOAD_SHED_CURRENT_VALUE,
                                        cpu_percent ) ) != 0 ){

             LE_send_msg( GL_INFO, "ORPGLOAD_set_data Failed (%d)\n",
                          ret );
             return( PS_DEF_FAILED );


          }
          */

       }

    }

    /* Return to application. */
    return (PS_DEF_SUCCESS);

/* End of MCPU_report_cpu_utilization() */
}

/* private function are define below. */
/************************************************************

   Description:
      Callback function for the MALRM_CPU_MONITOR timer
      expiration.

   Inputs:
      See malrm man page.

   Outputs:

   Returns:

   Notes:

************************************************************/
void Cpu_mon_alarm_callback( malrm_id_t alarm_id ){

   if( alarm_id == (malrm_id_t) MALRM_CPU_MONITOR )
      Psg_cpu_monitor_alarm_expired = 1;

/* End of Cpu_mon_alarm_callback() */
}

/******************************************************************

    Description:
       The RPG status message UN callback function.

    Input:
       See LB man page.
 
    Output:

    Returns:
       There is no return defined for this function.
	
******************************************************************/
static void Task_status_callback( int fd, LB_id_t msgid, int msg_info, 
                                  void *arg ){

    /* Set the status received flag. */
    Psg_cpu_info_avail = 1;

/* End of Task_status_callback() */
}

/*******************************************************************
   Description:
      This function performs a binary search of an array of 
      Cpu_stats_t entries based in task id.  If a match found,
      returns index into array.
  
   Inputs:
      array - pointer to array to search.
      size - size of array to search.
      task_name - the task to find.
  
   Returns:
      Returns index to entry in array, or -1 is entry not found.

   Notes:

******************************************************************/
static int Binary_search( Cpu_stats_t *array, int size, char *task_name ){

   int top, middle, bottom;
   
   /* Set the list top and bottom bounds. */
   top = size - 1;
   if( top == -1 )
      return (-1);
  
   bottom = 0;
   
   /* Do Until task_name found or task_name not in list. */
   while( top > bottom ){
  
      middle = (top + bottom)/2;
      if( strcmp( task_name, (char *) &((array + middle)->task_name[0])) > 0 )
         bottom = middle + 1;
  
      else if( strcmp( task_name, &((array + middle)->task_name[0]) ) == 0 )
         return ( middle );
  
      else
         top = middle;
  
   /* End of "while" loop. */
   }
   
   /* Modify the task status. */
   if( strcmp( (char *) &((array + top)->task_name[0]), task_name ) == 0 )
      return ( top );
   
   else
      return (-1);
   
/* End of Binary_search( ) */
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
  
      if( strcmp( (char *) &((array + index)->task_name[0]),
                  (char *) &((array + index - 1)->task_name[0]) ) < 0 ){

         current = *(array+index); 
         for( place = index-1; place >= 0; place-- ){

            *(array + place + 1) = *(array + place);
            if( (place == 0) 
                      ||
                (strcmp( (char *) &((array + place - 1)->task_name[0]),
                         (char *) &(current.task_name[0]) ) <= 0) )
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

         stats->next = (Proc_stats_t *) MISC_malloc( (size_t) sizeof(Proc_stats_t) );
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
