/*
 * Micro-benchmark fro Linux event -- Network interrupt on single core (server)
 *
 * Server setup
 *  SCHED_RR
 *  -- 1 thread (H) on core 4 with higher priority 98
 *  -- 1 thread (L) on core 4 with lower priority  19
 * Client setup (local test)
 *  -- 1 thread on core 1, sending UDP packets
 * 
 * Measure steps:
 -- server
 * 1) On server side, using SCHED_RR (RT prio 0~99)
 * 2) Assign H and L on core 4
 * 3) H's prio = 98 and L's prio = 19
 * 4) H opens tun device
 * 5) H blocks read on tun device (arrival packets)
 * 6) L keeps spinning
 rdtsc(start) in L
 * 6) when packets arrive and trigger the interrupt, H will continue to consume the data
 rdtsc(end) in H
 * 7) repeat 4) for fix times (e.g. MAX_UDP_PACKETS)
 * 8) finally compute the average cost of network interrupt

 -- client
 * 1) generating UDP packets and send to server (e.g. MAX_UDP_PACKETS)

 * Note: To run the test, 1) start server first by executing ./network_server
 *                        2) run ./run.sh on the different machine or cores
 *                        3) both need su
 * Jiguo Song
 *
*/

#define TUNDEV "tun0"
//#define IPADDR  "192.168.2.22"    // home
#define IPADDR  "192.168.2.22"    // cluster
//#define IPADDR  "128.164.157.249"   // lab
#define P2PPEER "10.0.2.8"

#include "../thread_settings.h"

/* /\* Scheduling Policies */
/*  *\/ */
/* #define SCHED_OTHER  0 */
/* #define SCHED_FIFO   1 */
/* #define SCHED_RR     2 */

volatile unsigned long long total_cost = 0;
#define NUM_THREADS  2
pthread_t  threads[NUM_THREADS];  // 0 low, 1 high

static int tun_create(char *dev, int flags)
{
	struct ifreq ifr;
	int fd, err;

	assert(dev != NULL);

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0) //open character device
		return fd;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags |= flags;
	if (*dev != '\0')
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	err = ioctl(fd, TUNSETIFF, (void *)&ifr);

	if (err < 0)
	{
		close(fd);
		return err;
	}
	strcpy(dev, ifr.ifr_name);

	return fd;
}

void *
lowest(void *arg) 
{
	struct sched_param param;
	int ret;

	move_thread_to_core(3);
	set_resource_limit();
	get_thread_info(pthread_self());

	sleep(1);
	while(1) {
		start = rdtsc();
		/* if (end) break; */
		/* pthread_yield(); */
	}

	pthread_exit(NULL);
}

void *
highest(void *arg)
{
	struct sched_param param;
	int tun, ret;
	char tun_name[IFNAMSIZ];
	unsigned char buf[4096] = {0};

	move_thread_to_core(3);
	set_resource_limit();
	get_thread_info(pthread_self());

	tun_name[0] = '\0';
	tun = tun_create(tun_name, IFF_TUN | IFF_NO_PI);
	if (tun < 0) some_error("tun_create");

	printf("TUN name is %s (pid %d)\n", tun_name, getpid());

        char tstr[] = "ifconfig "TUNDEV" inet "IPADDR" netmask 255.255.255.0 pointopoint "P2PPEER;
        printf("executing: \"%s\"\n", tstr);
        if (system(tstr) < 0) some_error("failed to boot up net device");

	sleep(1);

	int num = 0;
	while (num++ < MAX_UDP_PACKETS) {
		unsigned char ip[4];
		ret = read(tun, buf, sizeof(buf));
		if (ret < 0) assert(0);
		end = rdtsc();

		/* printf("start %llu end %llu\n", start, end); */
		/* printf("cost %llu\n", end - start); */
		total_cost += end - start;
	}
	
	printf("avg cost %llu (%d times)\n", total_cost/MAX_UDP_PACKETS, MAX_UDP_PACKETS);

	pthread_kill(threads[0], SIGQUIT);
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
	ret = pthread_create(&threads[0], NULL, (void *)lowest,NULL);
	if (ret) assert(0);

	ret = pthread_create(&threads[1], &attr_obj_high, (void *)highest,NULL);
	if (ret) assert(0);

	pthread_attr_destroy(&attr_obj_low);
	pthread_attr_destroy(&attr_obj_high);

	//wait for created threads die
	printf("join here\n");
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);

	return 0;
}

