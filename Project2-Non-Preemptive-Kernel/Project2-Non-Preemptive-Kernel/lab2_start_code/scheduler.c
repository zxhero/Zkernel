/* scheduler.c */

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "util.h"
#include "queue.h"

int scheduler_count;
// process or thread runs time
uint64_t cpu_time;

void printstr(char *s);
void printnum(unsigned long long n);
void scheduler(void)
{
	++scheduler_count;
	if(ready_queue->isEmpty == 0)
		current_running = queue_pop(ready_queue);
	else
		return;
	// pop new pcb off ready queue
	/* need student add */ 
}

void do_yield(void)
{
	save_pcb();
	queue_push(ready_queue,current_running);
	/* push the qurrently running process on ready queue */
	/* need student add */


	// call scheduler_entry to start next task
	scheduler_entry();

	// should never reach here
	ASSERT(0);
}

void do_exit(void)
{
	/* need student add */
	current_running->thd_state = PROCESS_EXITED;
	
	scheduler_entry();
}

void block(void)
{
	save_pcb();
	queue_push(blocked_queue,current_running);
	/* need student add */
	scheduler_entry();
	// should never reach here
	ASSERT(0);
}

int unblock(void)
{
	unblock_pcb = queue_pop(blocked_queue);
	queue_push(ready_queue,unblock_pcb);
	/* need student add */
}

bool_t blocked_tasks(void)
{
	return !blocked_queue->isEmpty;
}
