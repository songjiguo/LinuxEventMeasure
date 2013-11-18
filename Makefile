init:
	echo "-1" > /proc/sys/kernel/sched_rt_runtime_us
	echo "1000000000" > /proc/sys/kernel/sched_rt_period_us

all:
	@echo ""
	@echo "Compile benchmark --  network"
	cd network; make all
	@echo ""
	@echo "Compile benchmark --  tlbflush"
	cd tlb; make all
	@echo ""
	@echo "Compile benchmark --  pipe"
	cd pipe; make all
	@echo ""
	@echo "Compile benchmark --  timer"
	cd timer; make all
	@echo ""
	@echo "Compile benchmark --  cv"
	cd cv; make all

clean:
	cd network; make clean
	cd tlb; make clean
	cd pipe; make clean
	cd timer; make clean
	cd cv; make clean

