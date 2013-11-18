/*
 * Micro-benchmark fro Linux event -- Network interrupt on single core (client)
 *
 * All measurement steps are explained in server side 

 * Jiguo Song
 *
*/


#include "../thread_settings.h"

unsigned long long max = 0, min = (unsigned long long)-1, tot = 0, cnt = 0;
int rcv = 0;
char *rcv_msg;

unsigned int msg_sent = 0, msg_rcved;

void construct_header(char *msg)
{
	static unsigned int seqno = 1;
	unsigned long long time;
	unsigned int *int_msg = (unsigned int *)msg;
	unsigned long long *ll_msg = (unsigned long long *)msg;
	
	/* rdtscll(time); */
	int_msg[0] = seqno;
	ll_msg[1] = time;
//	printf("sending message %d with timestamp %lld\n", seqno, time);
	seqno++;

	return;
}


int main(int argc, char *argv[])
{
	int fd, fdr;
	struct sockaddr_in sa;
	int msg_size;
	char *msg;
	int sleep_val;

	move_self_to_core(0);

	if (argc != 5) {
		printf("Usage: %s <ip> <port> <msg size> <sleep_val> <opt:rcv_port>\n", argv[0]);
		return -1;
	}
	sleep_val = atoi(argv[4]);
	sleep_val = sleep_val == 0 ? 1 : sleep_val;

	msg_size = atoi(argv[3]);
	msg_size = msg_size < (sizeof(unsigned long long)*2) ? (sizeof(unsigned long long)*2) : msg_size;
	msg     = malloc(msg_size);
	rcv_msg = malloc(msg_size);

	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(atoi(argv[2]));
	sa.sin_addr.s_addr = inet_addr(argv[1]);
	if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) some_error("Establishing socket");

	int num = 0;
	while (num++ < MAX_UDP_PACKETS) {
		int i;
		
		/* construct_header(msg); */
		
		if (sendto(fd, msg, msg_size, 0, (struct sockaddr*)&sa, sizeof(sa)) < 0 &&
		    errno != EINTR) some_error("send error");
		msg_sent++;
		sleep(1);   // have server wait for the next packet
	}

	return 0;
}
