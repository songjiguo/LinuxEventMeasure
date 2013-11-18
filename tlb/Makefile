all: tlbflush

tlbflush: cores_tlbflush.c
	gcc -o $@ cores_tlbflush.c -lrt -lpthread -O3

clean:
	rm tlbflush

