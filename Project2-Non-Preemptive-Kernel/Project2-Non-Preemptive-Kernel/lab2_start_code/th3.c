#include "common.h"
#include "scheduler.h"
#include "util.h"
unsigned int cpu_cycle1;
unsigned int cpu_cycle2;
void thread4(void)
{
	int i = 0;
	uint32_t switch_time;
	while(1){ 
		if(i > 2){
			cpu_cycle2 = get_timer();
			switch_time = ((unsigned int)(cpu_cycle2 - cpu_cycle1)) / (2*MHZ);
			print_location(1, i);
			printstr("Process Swith Time (in us): ");
			printint(40, i, switch_time);
		}
		if(i > 20) break;
		i+= 2;
		cpu_cycle1 = get_timer();
		do_yield();
	}
	do_exit();
}

void thread5(void)
{
	int i = 1;
	uint32_t switch_time;
	while(1){ 
		cpu_cycle2 = get_timer();
		switch_time = ((unsigned int)(cpu_cycle2 - cpu_cycle1)) / MHZ;
		print_location(1, i);
		printstr("Thread Swith Time (in us): ");
		printint(40, i, switch_time);
		if(i > 20) break;
		i += 2;
		cpu_cycle1 = get_timer();
		do_yield();
	}
	do_exit();
}
