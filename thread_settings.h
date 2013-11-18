/*
 * Micro-benchmark fro Linux event -- Header Files and help functions
 *
 * Jiguo
*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

//client
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>

//server
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <stddef.h>
#include <net/if.h>


#define MAX_UDP_PACKETS  10 // send 10 times to server   -- network
#define MAX_CV_TIMES     500 // for average computation -- cv
#define MAX_PIPE_TIMES   100000// pipes
#define MAX_TIMER_TIMES   100// timer

volatile unsigned long long start, end;
static __inline__ unsigned long long
rdtsc(void)
{
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}

static
pid_t gettid(void)
{
	return syscall(__NR_gettid);
}

static void
get_sched_info()
{
	int scheduler_policy;
	struct sched_param my_params;

	scheduler_policy=sched_getscheduler(0);

	my_params.sched_priority=sched_get_priority_max(SCHED_RR);
	printf("SCHED_OTHER = %d SCHED_FIFO =%d SCHED_RR=%d \n",SCHED_OTHER,SCHED_FIFO,SCHED_RR);
	printf("the current scheduler = %d \n",scheduler_policy);

	return;
}

static void
some_error(char *arg)
{
	printf("msg: %s\n", arg);
	assert(0);
}

static void 
move_thread_to_core(int number)
{
	pthread_t tid = pthread_self();
	cpu_set_t set;

	/* printf("thread id is %d\n", tid); */
	CPU_ZERO(&set);
	CPU_SET(number, &set);
	
	if (pthread_setaffinity_np(tid, sizeof(set), &set)) some_error("failed to set affinity\n");

	return;
}

static void 
move_self_to_core(int number)
{
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(number, &set);
	
	if (sched_setaffinity(0, sizeof(set), &set)) some_error("failed to set affinity\n");

	return;
}

static void 
move_process_to_core(int number)
{
	pid_t pid = getpid();
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(number, &set);
	
	if (sched_setaffinity(pid, sizeof(set), &set)) some_error("failed to set affinity\n");

	return;
}

static void print_raw(void *addr, size_t size)
{
	unsigned char *base = addr;
	size_t i;
	for (i = 0; i < size; i ++) {
		printf("%02x", base[i]);
	}
}

static void
get_thread_info(pthread_t tid)
{
	int priority, policy, ret;
	struct sched_param param;

	ret = pthread_getschedparam (tid, &policy, &param);
	if (ret) assert(0);
	priority = param.sched_priority; 
	/* print_raw(&tid, sizeof(tid)); */
	printf("Thread %lu prio %d (policy %d)\n", tid, priority, policy);

	return;
}

static void
set_resource_limit()
{
	int ret;
	struct rlimit rl;
	rl.rlim_cur = RLIM_INFINITY;
	rl.rlim_max = RLIM_INFINITY;
        ret = setrlimit(RLIMIT_RTPRIO, &rl);
	if (ret) assert(0);
	
	return;
}

