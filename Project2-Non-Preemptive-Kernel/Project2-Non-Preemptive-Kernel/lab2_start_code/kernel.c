/*
   kernel.c
   the start of kernel
   */

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "th.h"
#include "util.h"
#include "queue.h"

#include "tasks.c"

volatile pcb_t *current_running;

queue_t ready_queue, blocked_queue;
pcb_t *ready_arr[NUM_TASKS];
pcb_t *blocked_arr[NUM_TASKS];
struct queue A,B;
pcb_t pro[NUM_TASKS];

/*
   this function is the entry point for the kernel
   It must be the first function in the file
   */

#define PORT3f8 0xbfe48000

 void printnum(unsigned long long n)
 {
   int i,j;
   unsigned char a[40];
   unsigned long port = PORT3f8;
   i=10000;
   while(i--);

   i = 0;
   do {
   a[i] = n % 16;
   n = n / 16;
   i++;
   }while(n);

  for (j=i-1;j>=0;j--) {
   if (a[j]>=10) {
      *(unsigned char*)port = 'a' + a[j] - 10;
    }else{
	*(unsigned char*)port = '0' + a[j];
   }
  }
  printstr("\r\n");
}

void _stat(void){
	int i,j;
	uint32_t stack_pos[9] = {0xa0884000, 0xa0885000, 0xa0886000, 0xa0887000, 0xa0888000, 0xa0889000, 0xa088a000, 0xa088b000, 0xa088c000};
	
	
	/* some scheduler queue initialize */
	/* need student add */
	clear_screen(0, 0, 30, 24);
	ready_queue = &A;
	blocked_queue = &B;
	queue_init(ready_queue);
	queue_init(blocked_queue);
	
	
	ready_queue->pcbs = ready_arr;
	ready_queue->capacity = NUM_TASKS;
	blocked_queue->capacity = NUM_TASKS;
	blocked_queue->pcbs = blocked_arr;
	for(i = 0;i < NUM_TASKS;i++){
		for(j = 0;j < 10;j++){
			pro[i].GPR[j] = 0;
		}
		pro[i].GPR[10] = task[i]->entry_point;
		pro[i].thd_state = PROCESS_READY;
		pro[i].PID = task[i]->task_type;
		pro[i].GPR[8] = stack_pos[i];
		print_hex(5*(i+1), 24, pro[i].GPR[8]);
		print_hex(5*(i+1), 25, pro[i].GPR[10]);
		
		if(!queue_push(ready_queue,pro+i)) 
					ASSERT(0);
	}
	
	/* Initialize the PCBs and the ready queue */
	/* need student add */

	/*Schedule the first task */
	scheduler_count = 0;
	scheduler_entry();

	/*We shouldn't ever get here */
	ASSERT(0);
}
