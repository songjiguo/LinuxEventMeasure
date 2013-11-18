/*
 * Micro-benchmark fro Linux event -- SMP pipe communication
 *
 * Note:
 * pipe() syscall will create a half-duplex pipe in POSIX. Two file descriptors are used to 
 * refer to the two ends of the pipe (fd[0] -> read end, fd[1] -> write end).
 * Data written to the write end of the pipe is buffered by the kernel until 
 * it is read from the read end of the pipe. For SMP, we need a pair of pipes to measure
 * the overhead of the pipe (can not rdtsc on the different core/thread)
 * 
 * System setup (pipe in threads/processes)
 *  -- writer thread (w_thd)
 *  -- reader thread (r_thd)
 *  -- 2 cores (core 0 and core 1)
 *  -- a pair of pipes (A and B)
 *
 * Measure steps:
 *  1) pin r_thd to the core 0
 *  2) pin w_thd to the core 1
 *  3) pipe A and B are empty initially
 *  rdtsc(start)
 *  4) w_thd writes 1 byte into the pipe A (will wait if there is no space)
 *  5) w_thd reads byte from pipe B (will block if r_thd has not written in pipe B yet)
 *  6) switch to r_thd
 *  7) r_thd reads from the pipe A (will wait if there is no data in the pipe A)
 *  8) r_thd writes 1 byte into the pipe B
 *  9) switch to w_thd
 *  10) w_thd reads from pipe B
 *  rdtsc(end)
 *
 *  Jiguo
 *
*/

#include "../thread_settings.h"

extern void set_prio (unsigned int);
extern void pthread_prio (pthread_t, unsigned int);

#define PIPE_THREADS
/* #define PIPE_PROCESS */

#define NUM_CORE 2
int cores[NUM_CORE];

#define PAGE_SIZE (sizeof(unsigned long long))
#define BUFFERSIZE PAGE_SIZE
#define SEND_BYTES 10U

int reader_buffer[BUFFERSIZE/sizeof(int)];
int writer_buffer[BUFFERSIZE/sizeof(int)];

int reader_fd, writer_fd;
int reader_fd_b, writer_fd_b;

static
void *pipe_reader(void *par)
{
	unsigned int ret;
	char info;
	char info_b = 'Z';

	printf("reader's pid is %d\n", gettid());
	move_thread_to_core(0);

	/* pthread_prio(pthread_self(), 1); */

	int num = 0;
	while (num++ < MAX_PIPE_TIMES) {
		ret = read(reader_fd, &info, 1);
		if (ret == 0) break;
		/* printf("reader read %c\n", info); */
		/* sleep(1); */
		ret = write(writer_fd_b, &info_b, 1);
		if (ret < 0) some_error("reader write failed");
		/* printf("reader write %c\n", info_b); */
	}

	return;
}

static
void *pipe_writer(void *par)
{
	unsigned int ret;
	char info = 'A';
	char info_b;
	unsigned long long sum = 0;
	
	printf("writer's pid is %d\n", gettid());
	move_thread_to_core(1);

	/* pthread_prio(pthread_self(), 1); */
	
	int num = 0;
	while (num++ < MAX_PIPE_TIMES) {
		start = rdtsc();
		ret = write(writer_fd, &info, 1);
		if (ret < 0) some_error("write");
		/* printf("writer write %c\n", info); */
		ret = read(reader_fd_b, &info_b, 1);
		if (ret == 0) break;
		/* printf("writer read %c\n", info_b); */
		end = rdtsc();
		/* printf ("NUM %d: %lld -- ",first,  (end - start)); */
		sum += (end-start);
	}
	printf("%lld (%d) times\n", (sum/MAX_PIPE_TIMES) >> 1, MAX_PIPE_TIMES);
	return;
}

static void
init_cores()
{
	int i;
	for (i = 0; i < NUM_CORE; i++){
		cores[i] = i;
	}
	return;
}

int main(int argc, char **argv)
{
	int ret;
	pthread_t r_thd, w_thd;
	pid_t pid;
	int pipes[2];
	int pipes_b[2];

	/* Initialize cores array*/
	init_cores();

	/* set up the pipe */
	ret = pipe(pipes);
	if (ret) some_error("failed to create pipe\n");
	ret = pipe(pipes_b);
	if (ret) some_error("failed to create pipe\n");

	reader_fd = pipes[0];
	writer_fd = pipes[1];

	reader_fd_b = pipes_b[0];
	writer_fd_b = pipes_b[1];

	/* set up the reader/writer threads  */
	if (pthread_create(&r_thd, NULL, (void *)pipe_reader, NULL)) some_error("failed to create reader thread\n");
	if (pthread_create(&w_thd, NULL, (void *)pipe_writer, NULL)) some_error("failed to create writer thread\n");

	(void) pthread_join(r_thd, NULL);
	(void) pthread_join(w_thd, NULL);

	return 0;
}
