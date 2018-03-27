/* queue.c */
/* Do not change this file */

#include "common.h"
#include "queue.h"
#include "scheduler.h"

void queue_init(node_t * queue){
  queue->prev = queue->next = queue;
}

node_t *dequeue(node_t * queue){
  node_t *item;
  
  item = queue->next;
  if (item == queue) {
    /* The queue is empty */
    item = NULL;
  } 
  else {
    /* Remove item from the queue */
    item->prev->next = item->next;
    item->next->prev = item->prev;
  }
  return item;
}

void enqueue(node_t * queue, node_t * item){
    item->prev = queue->prev;
    item->next = queue;
    item->prev->next = item;
    item->next->prev = item;
}

int is_empty(node_t *queue){
  if( queue->next == queue )
    return 1;
  else
    return 0;
}

node_t *peek(node_t *queue){
  if( queue->next == queue )
    return NULL;
  else
    return queue->next;
}

void sortqueue(node_t *queue){
	if(is_empty(queue));
	else{
		pcb_t *head;
		pcb_t	*test;
		node_t *nhead;
		node_t	*ntest;
		nhead = queue->prev;
		ntest = nhead->prev;
		head = (pcb_t*)nhead; 
		while(ntest != queue){
			test = (pcb_t*)ntest;
			if((head->priority > test->priority && head->round == test->round) || head->round < test->round){
				nhead->next->prev = ntest;
				ntest->next = nhead->next;
				nhead->next = ntest;
				nhead->prev = ntest->prev;
				ntest->prev = nhead;
				nhead->prev->next = nhead;
				ntest = nhead->prev;
			}
			else	break;
		}
	}	
}

