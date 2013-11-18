/*
 * Micro-benchmark fro Linux event -- timer interrupt
 *
 * System setup (only 1 thread on a core)
 *  -- 1 thread T on core 1 with the highest priority
 *
 * Measure steps:
 * 1) Set T with the highest priority and pin to one core
 * 2) Set the timer by which SIGALRM will trigger a handler later when timer interrupt occurs
 * 3) Let T spin
 rdtsc(start) in T
 * 3) Timer interrupt happens and triggers the handler
 rdtsc(end) in handler

 * This is adapted from Sched_benchmark/timer.c by Gabe
 *
 * Jiguo Song
 *
*/

#include "../thread_settings.h"

#define NUM_CORE     1
#define NUM_THREADS  NUM_CORE

#define NSEC_WAIT 1000

volatile int flag = 0;
volatile unsigned long long total_cost = 0;
/* extern void set_prio (unsigned int nice); */

void
handler(int sig, siginfo_t *si, void *v)
{
	flag = 1;
	return;
}

int
main(void)
{
	sigset_t ss;
	timer_t tid;
	struct sigaction sa;
	struct itimerspec t;

	/* set_prio(0); */

	sigfillset (&ss);
	sa.sa_handler = NULL;
	sa.sa_sigaction = handler;
	sa.sa_mask = ss;
	sa.sa_flags = SA_SIGINFO;
	sa.sa_restorer = NULL;
	sigaction (SIGALRM, &sa, NULL);

	timer_create(CLOCK_REALTIME, NULL, &tid);
	t.it_interval.tv_sec = 1;
	t.it_interval.tv_nsec = 0;
	t.it_value.tv_sec = 0;
	t.it_value.tv_nsec = NSEC_WAIT;

	int i;
	for (i = 0; i< MAX_TIMER_TIMES; i++) {
		timer_settime(tid, 0, &t, NULL);  // start timer
		while (1) {
			start = rdtsc();
			if (flag == 1) {
				end = rdtsc();
				break;
			}
		}
		total_cost += end - start;
		start = end = 0;
		flag = 0;
	}
	printf ("timer: avg cost %lld (%d times)\n", total_cost/MAX_TIMER_TIMES, MAX_TIMER_TIMES);

	return 0;
}
