/*   @(#)timer.h	1.1	07 Jul 1998	*/

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
 * timer.h of snet module
 *
 * SpiderX25
 * @(#)$Id: timer.h,v 1.1 2000/02/25 17:15:06 john Exp $
 * 
 * SpiderX25 Release 8
 */


/* Lock out clock ISR */
#define splclock()   splhi()

/* Timers header, used to process expiries */
typedef struct thead
{
    void         (*th_expfunc)();
    caddr_t        th_exparg;
    struct timer  *th_expired;
} thead_t;

/* Individual timer */
typedef struct timer
{
    unchar         tm_id;
    unchar         tm_offset;
    ushort         tm_left;
    struct timer  *tm_next;
    struct timer **tm_back;
} s_timer_t;


/* External definitions */
extern void init_timers(thead_t *, s_timer_t *, int, void (*)(caddr_t ), caddr_t);
extern void stop_timers(thead_t *, s_timer_t *, int);
extern void set_timer(s_timer_t *, unsigned short);
extern void run_timer(s_timer_t *, unsigned short);
