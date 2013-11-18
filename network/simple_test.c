#define TUNDEV "tun0"
#define IPADDR  "192.168.1.22"
//#define IPADDR  "128.164.157.249"   // lab
#define P2PPEER "10.0.2.8"

#include "../thread_settings.h"

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
highest(void *arg)
{
	struct sched_param param;
	int tun, ret;
	char tun_name[IFNAMSIZ];
	unsigned char buf[4096] = {0};

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
		printf("read 1\n");
		ret = read(tun, buf, sizeof(buf));
		printf("read 2\n");
		if (ret < 0) assert(0);
		end = rdtsc();

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

	prio_high = sched_get_priority_max(SCHED_RR) - 1;

	param_high.sched_priority  = prio_high;
	
	//init attributes
	ret = pthread_attr_init(&attr_obj_high);
	if (ret) assert(0);

	//thread can specify its own scheduling policy
	ret = pthread_attr_setinheritsched(&attr_obj_high, PTHREAD_EXPLICIT_SCHED);
	if (ret) assert(0);

	//set scheduling policy
	ret = pthread_attr_setschedpolicy(&attr_obj_high, SCHED_RR);
  	if (ret) assert(0);

	//set attribute
	ret = pthread_attr_setschedparam(&attr_obj_high, &param_high);
	if (ret) assert(0);


	//check the thread priority info
	ret = pthread_attr_getschedparam(&attr_obj_high, &param_high);
	if (ret) assert(0);
	printf("obj2: thread priority : %d\n",param_high.sched_priority);

	//create threads
	ret = pthread_create(&threads[1], &attr_obj_high, (void *)highest,NULL);
	if (ret) assert(0);

	pthread_attr_destroy(&attr_obj_high);

	//wait for created threads die
	pthread_join(threads[1], NULL);

	return 0;
}

