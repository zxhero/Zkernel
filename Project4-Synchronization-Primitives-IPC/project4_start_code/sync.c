
#include "common.h"
#include "interrupt.h"
#include "queue.h"
#include "scheduler.h"
#include "sync.h"
#include "util.h"
#include "printf.h"

/*TODO operate */
static bool_t unblock_one(node_t *wait_enqueue)
{
	ASSERT(current_running->nested_count);

	pcb_t *pcb = NULL; 
	pcb = (pcb_t *)dequeue(wait_enqueue);
	if(pcb != NULL)
	{
		unblock(pcb);
		return TRUE;
	}
	return FALSE;
}

/* Initialize a lock */
void lock_init(lock_t * l){
    /* No critical section; it is the caller's responsibility to make sure
       that locks are initialized only once */
    l->status = UNLOCKED;
    queue_init(&l->wait_queue);
    l->held_task = NULL;
}

static int lock_acquire_helper(lock_t * l){
  ASSERT(disable_count);
  if (LOCKED == l->status) {
    current_running->blocking_lock = (void*)l;
    pcb_t* cur_task = current_running;
    lock_t* cur_lock;
    while (cur_task){
      pcb_t* tmp_task;
      cur_lock = (lock_t*)cur_task->blocking_lock;
      if (cur_lock){
        tmp_task = cur_lock->held_task;
        if (tmp_task==current_running){					//死锁发生
          current_running->blocking_lock = NULL;
          return 1;
        }
      }
      else break;
      cur_task = tmp_task;
    }
    block(&l->wait_queue);								//未分配到锁则阻塞
    current_running->blocking_lock = NULL;
  } else {
    l->status = LOCKED;
  }
  //current_running->held_lock = (void*)l;
	//print_hex(10, 24, l);
	//print_hex(10, 24, current_running->held_lock);
  return 0;
}

// Return 0 on succes
// Return 1 on failure due to deadlock (extra credit)
int lock_acquire(lock_t * l){
    enter_critical();
    int result = lock_acquire_helper(l);
    leave_critical();

    return result;
}

static void lock_release_helper(lock_t * l){
  ASSERT(disable_count);
  if (!unblock_one(&l->wait_queue)) {
    l->status = UNLOCKED;
	l->held_task = NULL;
  }
	else	l->held_task = (pcb_t*)(ready_queue.prev);									//？？？？l
}

void lock_release(lock_t * l){
    enter_critical();
    lock_release_helper(l);
    leave_critical();
}

/* TODO: Initialize a condition variable */
void condition_init(condition_t * c){
	//enter_critical();
	queue_init(&(c->pro_wait_queue));
	//leave_critical();
}

/* TODO: Release lock m and block the thread (enqueued on c).  When unblocked,
   re-acquire m */
/* error */
void condition_wait(lock_t * m, condition_t * c){
	lock_release(m);
	enter_critical();
	current_running->condition_wait_mutex = (void*)m;
	block(&c->pro_wait_queue);
	leave_critical();
	lock_acquire((lock_t*)current_running->condition_wait_mutex);
	current_running->condition_wait_mutex = NULL;
}

/* TODO: Unblock the first thread waiting on c, if it exists */
void condition_signal(condition_t * c){
	enter_critical();
	if(!unblock_one(&c->pro_wait_queue));
	leave_critical();
}

/* TODO: Unblock all threads waiting on c */
void condition_broadcast(condition_t * c){
	enter_critical();
	while(unblock_one(&c->pro_wait_queue));
	leave_critical();
}

/* TODO: Initialize a semaphore with the specified value. value must be >= 0 */
void semaphore_init(semaphore_t * s, int value){
	//enter_critical();
	s->value = value;
	queue_init(&(s->pro_wait_queue));
	lock_init(&(s->mutex));
	//leave_critical();
}

/* TODO: Increment the semaphore value atomically */
void semaphore_up(semaphore_t * s){
	
	lock_acquire(&(s->mutex));
	enter_critical();
	if(s->value > 0){
		s->value = s->value + 1;
	}
	else{
		if(unblock_one(&(s->pro_wait_queue)));
		else{
			s->value = s->value + 1;
		}
	}
	//print_hex(10, 24, (uint32_t)(s->value));
	leave_critical();
	lock_release(&(s->mutex));
	
}

/* TODO: Block until the semaphore value is greater than zero and decrement it */
void semaphore_down(semaphore_t * s){
	
	lock_acquire(&(s->mutex));
	enter_critical();
	
	if(s->value > 0){
		s->value = s->value - 1;
		leave_critical();
		lock_release(&(s->mutex));
	}
	else{
		leave_critical();
		lock_release(&(s->mutex));
		enter_critical();
		//print_hex(10, 24, (uint32_t)(s->value));
		block(&(s->pro_wait_queue));
		leave_critical();
	}
	
}

/* TODO: Initialize a barrier.  n is number of threads that rendezvous at the
   barrier */
void barrier_init(barrier_t * b, int n){
	//enter_critical();
	b->total_num = n;
	b->cur_num = 0;
	queue_init(&(b->pro_wait_queue));
	lock_init(&b->mutex);
	//leave_critical();
}

/* TODO: Block until all n threads have called barrier_wait */
void barrier_wait(barrier_t * b){
	//lock_acquire(&b->mutex);
	enter_critical();
	
	b->cur_num++;
	if(b->cur_num == b->total_num){
		while(unblock_one(&b->pro_wait_queue));
		b->cur_num = 0;
	}
	else{
		block(&b->pro_wait_queue);
	}
	
	leave_critical();
	//lock_release(&b->mutex);
}
