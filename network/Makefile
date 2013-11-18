init:
	echo "-1" > /proc/sys/kernel/sched_rt_runtime_us
	echo "1000000000" > /proc/sys/kernel/sched_rt_period_us

all: network_server  network_client

network_server: core_network.c
	gcc -o $@ core_network.c -lrt -lpthread -O3

network_client: udp_client.c
	gcc -o $@ udp_client.c -lrt -O3

clean:
	rm network_server network_client

