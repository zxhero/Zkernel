/* Author(s): <Your name here>
 * COS 318, Fall 2013: Project 3 Pre-emptive Scheduler
 * Implementation of the process scheduler for the kernel.
 */

#include "common.h"
#include "interrupt.h"
#include "queue.h"
#include "printf.h"
#include "scheduler.h"
#include "util.h"
#include "syslib.h"

pcb_t *current_running;
node_t ready_queue;
node_t sleep_wait_queue;
// more variables...
volatile uint64_t time_elapsed=0;

/* TODO:wake up sleeping processes whose deadlines have passed */
void check_sleeping(){
	node_t* i = (sleep_wait_queue.next);
	node_t* j = (&(sleep_wait_queue));
	pcb_t* s;
	while (1) {
		if (i == (&(sleep_wait_queue)) || i == NULL)	break;
		else if(((pcb_t*)i) ->deadline <= time_elapsed){
			//s = (pcb_t*)dequeue(&sleep_wait_queue);
			s= (pcb_t*)dequeue(((node_t*)i)->prev);
			s->status = READY;
			//((pcb_t*)j)->deadline = 0;
			if(s -> priority == 0 ) {
				s -> priority = s -> ori_priority;
				s -> round++;
			}
			enqueue(&ready_queue, ((node_t*)s));
			sortqueue(&ready_queue);
			//i = peek(j);
			break;
		}
		else{
			j = peek(j);
			i = peek(((node_t*)i));
			//printf(9,10, "CAN NOT WAKE, IS EMPTY? %d",is_empty(&sleep_wait_queue));
		}
		
		//i = i -> next;
		//printf(9,10, "time_elapsed %d seconds", (int)time_elapsed);
	}
}

/* Round-robin scheduling: Save current_running before preempting */
void put_current_running(){
	check_sleeping();
	current_running ->status = READY;
	if(current_running -> priority == 0 ) {
		current_running -> priority = current_running -> ori_priority;
		current_running -> round++;
	}
	enqueue(&ready_queue, (node_t*)current_running);
	sortqueue(&ready_queue);
}

/* Change current_running to the next task */
void scheduler(){
     ASSERT(disable_count);
	int i;
	//print_hex(30, 20, disable_count);
     check_sleeping(); // wake up sleeping processes
	if(current_running -> pid !=0 && current_running->status == READY){
		put_current_running();
	}
	//check_sleeping(); 
     while (is_empty(&ready_queue)){

          leave_critical();
	//printf(9,10, "time_elapsed %d ", (int)time_elapsed);
	//	for(;;);
          enter_critical();
          check_sleeping();
     }
     current_running = (pcb_t *) dequeue(&ready_queue);
     ASSERT(NULL != current_running);
	//printf(24,50, "pid = ", current_running->pid);
	//printstr("WRIGHT ");]

     current_running->entry_count = current_running->entry_count+1;
	do_setpriority(do_getpriority()-1);
}

int lte_deadline(node_t *a, node_t *b) {
     pcb_t *x = (pcb_t *)a;
     pcb_t *y = (pcb_t *)b;

     if (x->deadline <= y->deadline) {
          return 1;
     } else {
          return 0;
     }
}

void do_sleep(int milliseconds){
     ASSERT(!disable_count);

     enter_critical();
     // TODO
	//printf(15,20, "milliseconds %d ", milliseconds);
	 current_running->deadline = time_elapsed + (uint64_t)(milliseconds);
	//print_hex(30, 10, current_running->deadline);
	//printf(9,20, "deadline %d ", current_running->deadline);
	 current_running->status = SLEEPING;
	 enqueue(&sleep_wait_queue, (node_t *)current_running);
	//current_running = (pcb_t*)current_running;
	 scheduler_entry();
}

void do_yield(){
     enter_critical();
	//put_current_running();
     scheduler_entry();
	//leave_critical();
}

void do_exit(){
     enter_critical();
     current_running->status = EXITED;
     scheduler_entry();
     /* No need for leave_critical() since scheduler_entry() never returns */
}

void block(node_t * wait_queue){
     ASSERT(disable_count);
     current_running->status = BLOCKED;
     enqueue(wait_queue, (node_t *) current_running);
     scheduler_entry();
     enter_critical();
}

void unblock(pcb_t * task){
     ASSERT(disable_count);
     task->status = READY;
     enqueue(&ready_queue, (node_t *) task);
}

pid_t do_getpid(){
     pid_t pid;
     enter_critical();
     pid = current_running->pid;
     leave_critical();
     return pid;
}

uint64_t do_gettimeofday(void){
     return time_elapsed;
}

priority_t do_getpriority(){
	/* TODO */
	return current_running->priority;
}


void do_setpriority(priority_t priority){
	/* TODO */
	current_running->priority = priority; 
}

uint64_t get_timer(void) {
     return do_gettimeofday();
}
