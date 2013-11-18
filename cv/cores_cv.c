/*
 * Micro-benchmark fro Linux event -- SMP conditional variable for 2 cores
 *
 * System setup (one thread on each core)
 *  -- 1 thread (H) on core 1 with higher priority
 *  -- 1 thread (L) on core 2 with lower priority
 *  -- 2 conditional variables (cv1 and cv2)
 *  -- 2 mutex (mutex1 and mutex2)
 *
 * Measure steps:
 * 1) Assign each thread to a core (H on core 1 and L on core 2)
 * 2) H takes mutex1
 * 3) H waits on cv1 (releases mutex1 at the same time)
 * 4) L takes mutex1
 rdtsc(start1) in L
 * 5) L sends signal to cv1
 * 6) L releases mutex1
 * 7) L takes mutex2
 * 8) L waits on cv2 (releases mutex2 at the same time)
 rdtsc(end1) in H
 * 9) H takes mutex2
 rdtsc(start2) in H
 *10) H sends signal to cv2
 *11) H releases mutex2 and block
 rdtsc(end2) in L
 *12) L releases mutex2 and block
 *13) H terminates and L terminates

 * Note: end1-start1+end2-start2 == end2-start1+end1-start2 (rdtsc on the same cores) 

 * Jiguo Song
 *
*/

#include "../thread_settings.h"

#define NUM_CORE     2
#define NUM_THREADS  NUM_CORE

pthread_t  threads[NUM_THREADS];
int       rr_max_priority;

int count = 0;

pthread_mutex_t count_mutex1, count_mutex2;
pthread_cond_t count_threshold_cv1, count_threshold_cv2;

volatile unsigned long long total_cost;
// we need a pair of cv to compute average cost over 2 cores
volatile unsigned long long start1, end1;
volatile unsigned long long start2, end2;

void *
lowest(void *arg) 
{
	move_thread_to_core(2);
	set_resource_limit();
	get_thread_info(pthread_self());

	int i;
	for (i = 0; i < MAX_CV_TIMES; i++) {
		pthread_mutex_lock(&count_mutex1);
		count++;
		start1 = rdtsc();
		pthread_cond_signal(&count_threshold_cv1);
		pthread_mutex_unlock(&count_mutex1);
		
		pthread_mutex_lock(&count_mutex2);
		pthread_cond_wait(&count_threshold_cv2, &count_mutex2);
		end2 = rdtsc();

		total_cost += end1-start1+end2-start2;

		count++;
		pthread_mutex_unlock(&count_mutex2);
	}

	usleep(10);

	pthread_exit(NULL);
}

void *
highest(void *arg) 
{
	move_thread_to_core(1);
	set_resource_limit();
	get_thread_info(pthread_self());

	int i;
	for (i = 0; i < MAX_CV_TIMES; i++) {
		pthread_mutex_lock(&count_mutex1);
		pthread_cond_wait(&count_threshold_cv1, &count_mutex1);
		end1 = rdtsc();
		count++;
		pthread_mutex_unlock(&count_mutex1);
		
		pthread_mutex_lock(&count_mutex2);
		count++;
		start2 = rdtsc();
		pthread_cond_signal(&count_threshold_cv2);
		pthread_mutex_unlock(&count_mutex2);
	}
	
	usleep(10);
	pthread_exit(NULL);
}

int 
main(int argc, char *argv[])
{
	int prio_low, prio_high;
	struct sched_param param_low, param_high;
	pthread_attr_t attr_obj_low, attr_obj_high;

	int ret;

	move_self_to_core(1);
	printf("PID %d\n", getpid());

	prio_low  = sched_get_priority_max(SCHED_RR) - 80;
	prio_high = sched_get_priority_max(SCHED_RR) - 1;

	param_low.sched_priority  = prio_low;
	param_high.sched_priority  = prio_high;

	//init attributes
	ret = pthread_attr_init(&attr_obj_low);
	if (ret) assert(0);
	ret = pthread_attr_init(&attr_obj_high);
	if (ret) assert(0);

	//thread can specify its own scheduling policy
	ret = pthread_attr_setinheritsched(&attr_obj_low, PTHREAD_EXPLICIT_SCHED);
	if (ret) assert(0);
	ret = pthread_attr_setinheritsched(&attr_obj_high, PTHREAD_EXPLICIT_SCHED);
	if (ret) assert(0);

	//set scheduling policy
	ret = pthread_attr_setschedpolicy(&attr_obj_low, SCHED_RR);
	if (ret) assert(0);
	ret = pthread_attr_setschedpolicy(&attr_obj_high, SCHED_RR);
  	if (ret) assert(0);

	//set attribute
	ret = pthread_attr_setschedparam(&attr_obj_low, &param_low);
	if (ret) assert(0);
	ret = pthread_attr_setschedparam(&attr_obj_high, &param_high);
	if (ret) assert(0);

	//check the thread priority info
	ret = pthread_attr_getschedparam(&attr_obj_low, &param_low);
	if (ret) assert(0);
	ret = pthread_attr_getschedparam(&attr_obj_high, &param_high);
	if (ret) assert(0);
	printf("obj1: thread priority : %d\n",param_low.sched_priority);
	printf("obj2: thread priority : %d\n",param_high.sched_priority);

	//create threads
	/* ret = pthread_create(&threads[0], &attr_obj_low, (void *)lowest,NULL); */
	ret = pthread_create(&threads[1], &attr_obj_high, (void *)highest,NULL);
	if (ret) assert(0);
	sleep(1);

	ret = pthread_create(&threads[0], &attr_obj_low, (void *)lowest,NULL);
	if (ret) assert(0);
	sleep(1);

	pthread_mutex_init(&count_mutex1, NULL);
	pthread_mutex_init(&count_mutex2, NULL);
	pthread_cond_init (&count_threshold_cv1, NULL);
	pthread_cond_init (&count_threshold_cv2, NULL);

	/* printf("cost %llu\n", end2-start1+end1-start2); */
	/* printf("SMP CV Cost: %llu\n", (end1-start1+end2-start2) >> 1); */
	printf("cv cost: %llu (%d times)\n", (total_cost/MAX_CV_TIMES) >> 1, MAX_CV_TIMES);

	int i;
	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	return 0;
}
