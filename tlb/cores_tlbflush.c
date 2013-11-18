/*
 * Micro-benchmark fro Linux event -- SMP TLB shootdown
 *
 * Note:
 * TLB shootdown procedure (When the access permission to a page is modified. e.g munmap)
 * 1) core 0 locks down the page table
 * 2) find the set of affected cores
 * 3) send IPI to those cores
 * 4) remote cores execute ISR for TLB invalidation
 * 5) then send acknowledgment msg back to core 0
 * 6) core 0 releases lock and continue the execution

 * System setup (one thread on each core)
 *  -- up to 80 cores
 *  -- 1 thread on core 0 (main thread)
 *  -- 79 threads on other 79 cores
 *
 * Measure steps: (measure the overhead of above TLB shootdown step 1 to 6)
 * 1) Assign each thread to a core (1 main thread an rest are workers)
 * 2) N*Pages shared among all cores (e.g all threads) from main thread
 * 3) main thread touches the shared region
 * 4) Each thread touches the shared region
 * 5) rdtsc(start)
 * 6) main thread munmap the shared memory
 * 7) rdtsc(end)
 * 8) overhead of TLB shootdown (e.g. IPI) = end-start
 *
 * Jiguo Song
 *
*/

#include "../thread_settings.h"

#define PAGE_SIZE 	4096
#define PAGE_NUM        1
#define MAX_RANGE	PAGE_NUM*PAGE_SIZE

#define NUM_ACCESS      32	       /* how many times to access shared memory by a thread, not used now*/
#define NUM_CORE        4

volatile int threadstart;       // control when the created threads start accessing the mem
int cores[NUM_CORE];

//data for threads
struct thd_data{
	int *p;
	void *start_addr;
	int core;
};

static void
init_cores()
{
	int i;
	for (i = 1; i < NUM_CORE; i++){
		cores[i] = i;
	}

	return;
}

//each thread accessing the memory
void *accessmm(void *data){
	struct thd_data *d = data;
	char x;
	int i, k;
	int randn[PAGE_SIZE];
	
	assert(d);
	printf("pid is %d on core %d\n", gettid(), d->core);

	move_thread_to_core(d->core);

	cpu_set_t l_cpuSet;
	if (pthread_getaffinity_np(pthread_self()  , sizeof( cpu_set_t ), &l_cpuSet ) == 0 )
		for (i = 0; i < NUM_CORE; i++)
			if (CPU_ISSET(i, &l_cpuSet))
				printf("XXXCPU: CPU %d\n", i);


	//access whole memory, will generate many page faults 
	volatile void *tempaddr;
	for (tempaddr = d->start_addr; tempaddr < d->start_addr + MAX_RANGE; tempaddr += PAGE_SIZE)
		memset((char *)tempaddr, 1, PAGE_SIZE);

	/* for (i=0;i<PAGE_SIZE; i++) */
	/* 	randn[i] = rand(); */

	/* for (k=0; k < *d->p; k++) { */
	/* 	/\* x = *(volatile char *)(d->start_addr + randn[k]%MAX_RANGE);  // read *\/ */
	/* 	*(char *)(d->start_addr + randn[k]%MAX_RANGE) = 1;    //write */
	/* } */
	
	
	while (threadstart == 0 ) usleep(1);
	printf("done\n");
	return;
}

int main(int argc, char *argv[])
{
	int i;
	int page = NUM_ACCESS;
	int randn[PAGE_SIZE];
	pthread_t pid[NUM_CORE];  // each core has exact one thread
	void *start_addr;
	struct thd_data data;
	
	cpu_set_t l_cpuSet;
	int ret;
	move_self_to_core(0);  // make sure we are on core 0
	/* ret = sched_getaffinity(0, sizeof(cpu_set_t), &l_cpuSet); */
	/* printf("sched_getaffinity %d\n", ret); */
	
	/* Initialize cores array*/
	init_cores();
	
	/* move_process_to_core(getpid(), 0); */

	start_addr = mmap(0, MAX_RANGE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	if (start_addr == MAP_FAILED) assert(0);
	
	//access whole memory, will generate many page faults 
	volatile void *tempaddr;
	for (tempaddr = start_addr; tempaddr < start_addr + MAX_RANGE; tempaddr += PAGE_SIZE)
		memset((char *)tempaddr, 0, PAGE_SIZE);

	threadstart = 0;
	data.p = &page; 
	data.start_addr = start_addr; 

	for (i = 1; i < NUM_CORE; i++) {
		data.core = cores[i];
		if(pthread_create(&pid[i], NULL, accessmm, &data)) assert(0);
		sleep(1);
	}

	sleep(2);  // make sure shared mem is touched by thread on other cores
	start = rdtsc();
	if(munmap(start_addr, MAX_RANGE) == -1) assert(0);
	end = rdtsc();

	threadstart = 1;   // terminate other threads now
	//get threads' result.
	for (i=1; i< NUM_CORE; i++) {
		pthread_join(pid[i], NULL);
	}
	printf("munmap use %llu \n",end-start);
	return 0;
}
