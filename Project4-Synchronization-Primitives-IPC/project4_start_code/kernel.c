	/* kernel.c: Kernel with pre-emptive scheduler */
/* Do not change this file */

#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "queue.h"
#include "scheduler.h"
#include "util.h"
#include "printf.h"
#include "printk.h"
#include "ramdisk.h"
#include "mbox.h"

#include "files.h"

#define NUM_TASKS 	(32)
#define NUM_PCBS      (NUM_TASKS)

pcb_t pcb[NUM_PCBS];
lock_t	mux[NUM_PCBS];

/* This is the system call table, used in interrupt.c */
int (*syscall[NUM_SYSCALLS]) ();

static uint32_t *stack_new(void);
static void first_entry(void);
static int invalid_syscall(void);
static void init_syscalls(void);
static void init_serial(void);
static void initialize_pcb(pcb_t *p, pid_t pid, struct task_info *ti);
static int do_spawn(const char *filename);
static int do_kill(pid_t pid);
static int do_wait(pid_t pid);
static int	do_lock_acquire(mutex_t l);
static void	do_lock_init(mutex_t l);
static void	do_lock_release(mutex_t l);

extern void asm_start();

void printcharc(char ch);

void __attribute__((section(".entry_function"))) _start(void)
{
	asm_start();
	static pcb_t garbage_registers;
	int i;

	clear_screen(0, 0, 80, 30);

	queue_init(&ready_queue);
	queue_init(&sleep_wait_queue);
	current_running = &garbage_registers;

	/* Transcribe the task[] array (in tasks.c)
	 * into our pcb[] array.
	 */
	for (i = 0; i < NUM_TASKS; ++i) {
		pcb[i].status = EXITED;
	}

	init_syscalls();
	init_interrupts();

	srand(get_timer()); /* using a random value */
	init_mbox();

	do_spawn("init");
	//print_status();
	/* Schedule the first task */
	enter_critical();
	scheduler_entry();
	//print_status();
	/* We shouldn't ever get here */
	ASSERT(FALSE);
}

static uint32_t *stack_new()
{
	static uint32_t next_stack = 0xa0f00000;

	next_stack += 0x10000;
	//ASSERT(next_stack <= 0xa1000000);
	return (uint32_t *) next_stack;
}

static void initialize_pcb(pcb_t *p, pid_t pid, struct task_info *ti)
{
	int	s = p->status;
	p->entry_point = ti->entry_point;
	p->pid = pid;
	p->task_type = ti->task_type;
	p->priority = 1;
	p->status = FIRST_TIME;
	switch (ti->task_type) {
		case KERNEL_THREAD:
			if(s == KILLED) p->kernel_tf.regs[29] =p->stack_k;
			else 	p->kernel_tf.regs[29] = (uint32_t)stack_new();
			p->nested_count = 1;
			break;
		case PROCESS:
			if(s == KILLED){
				p->kernel_tf.regs[29] = p->stack_k;
				p->user_tf.regs[29] =  p->stack_u;
			}
			else{
				p->kernel_tf.regs[29] =(uint32_t)stack_new();
				p->user_tf.regs[29] = (uint32_t)stack_new();
			}
			p->nested_count = 0;
			break;
		default:
			ASSERT(FALSE);
	}
	p->stack_k = p->kernel_tf.regs[29];
	p->stack_u = p->user_tf.regs[29];
	p->wait_in = NULL;
	p->condition_wait_mutex = NULL;
	queue_init(&(p->wait_queue));
	p->kernel_tf.regs[31] = (uint32_t) first_entry;
	p->lock_num = 0;
	//print_hex(10, 24, p->entry_point);
}


static void first_entry()
{
	uint32_t *stack, entry_point;

	enter_critical();

	if (KERNEL_THREAD == current_running->task_type) {
		stack = current_running->kernel_tf.regs[29];
	} else {
		stack = current_running->user_tf.regs[29];
	}
	entry_point = current_running->entry_point;
	//print_hex(10, 24, entry_point);
	// Messing with %esp in C is usually a VERY BAD IDEA
	// It is safe in this case because both variables are
	// loaded into registers before the stack change, and
	// because we jmp before leaving asm()
	asm volatile ("add $sp, $0, %0\n"
			"jal leave_critical\n"
			"nop\n"
			"add $ra, $0, %1\n"
			"jr $ra\n"
			:: "r" (stack), "r" (entry_point));

	ASSERT(FALSE);
}


static int invalid_syscall(void)
{
	HALT("Invalid system call");
	/* Never get here */
	return 0;
}


/* Called by kernel to assign a system call handler to the array of
   system calls. */
static void init_syscalls()
{
	int fn;

	for (fn = 0; fn < NUM_SYSCALLS; ++fn) {
		syscall[fn] = &invalid_syscall;
	}
	syscall[SYSCALL_YIELD] = (int (*)()) &do_yield;
	syscall[SYSCALL_EXIT] = (int (*)()) &do_exit;
	syscall[SYSCALL_GETPID] = &do_getpid;
	syscall[SYSCALL_GETPRIORITY] = &do_getpriority;
	syscall[SYSCALL_SETPRIORITY] = (int (*)()) &do_setpriority;
	syscall[SYSCALL_SLEEP] = (int (*)()) &do_sleep;
	syscall[SYSCALL_SHUTDOWN] = (int (*)()) &do_shutdown;
	syscall[SYSCALL_WRITE_SERIAL] = (int (*)()) &do_write_serial;
	syscall[SYSCALL_PRINT_CHAR] = (int (*)()) &print_char;//8
	syscall[SYSCALL_SPAWN] = (int (*)()) &do_spawn;
	syscall[SYSCALL_KILL] = (int (*)()) &do_kill;//10
	syscall[SYSCALL_WAIT] = (int (*)()) &do_wait;//11
    syscall[SYSCALL_MBOX_OPEN] = (int (*)()) &do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE] = (int (*)()) &do_mbox_close;
    syscall[SYSCALL_MBOX_SEND] = (int (*)()) &do_mbox_send;
    syscall[SYSCALL_MBOX_RECV] = (int (*)()) &do_mbox_recv;
	syscall[SYSCALL_TIMER] = (int (*)()) &get_timer;
	syscall[SYSCALL_LOCK_ACQ] = (int (*)()) &do_lock_acquire;
	syscall[SYSCALL_LOCK_INIT] = (int (*)()) &do_lock_init;
	syscall[SYSCALL_LOCK_REL] = (int (*)()) &do_lock_release;
}

/* Used for debugging */
void print_status(void)
{
	static char *status[] = { "First  ", "Ready", "Blocked", "Exited ", "Sleeping" ,"KILLED"};
	int i, base;

	base = 17;
	printf(base - 4, 6, "P R O C E S S   S T A T U S");
	printf(base - 2, 1, "Pid\tType\tPrio\tStatus\tEntries");
	for (i = 0; i < NUM_PCBS && (base + i) < 25; i++) {
		printf(base + i, 1, "%d\t%s\t%d\t%s\t%d", pcb[i].pid,
				pcb[i].nested_count == KERNEL_THREAD ? "Process" : "Thread",
				pcb[i].priority, status[pcb[i].status], pcb[i].entry_count);
	}
}

void do_shutdown(void)
{
	/* These numbers will work for bochs
	 * provided it was compiled WITH acpi.
	 * the default ubuntu 9 version of bochs
	 * is NOT compiled with acpi support, though
	 * the version in the Friend center lab DO have it.
	 * This will probably not work with
	 * any real computer.
	 */
	//outw( 0xB004, 0x0 | 0x2000 );

	/* Failing that... */
	HALT("Shutdown");
}

#define PORT3f8 0xbfe48000
#define PORT3fd 0xbfe48006

/* Write a byte to the 0-th serial port */
void do_write_serial(int character)
{

	// wait until port is free
	unsigned long port = PORT3f8;
	int i = 50000;
	while (i--);
	*(unsigned char*)port = character;

	//leave_critical();
}

void printcharc(char ch)
{
	do_write_serial(ch);
}

int print_char(int line, int col, char c){
	unsigned long port = PORT3f8;
	print_location(line, col);

	*(unsigned char *)port = c;

}

int spawn_times = 0;
static int do_spawn(const char *filename)
{
  (void)filename;
  /* TODO */
enter_critical();
  int	i;
  struct	task_info	tasks;
  File *file;
  file = ramdisk_find_File(filename);
  tasks.entry_point = (uint32_t)file->process;
  tasks.task_type = file->task_type;
	for (i = 0; i < NUM_PCBS; ++i) {
		if( pcb[i].status == KILLED){
			initialize_pcb(&(pcb[i]), (pid_t)i, &tasks);
			enqueue(&ready_queue, (node_t *)(&(pcb[i])));
			break;
		}
		else if(pcb[i].status == EXITED){
			initialize_pcb(&(pcb[i]), (pid_t)i, &tasks);
			enqueue(&ready_queue, (node_t *)(&(pcb[i])));
			break;
		}
	}
  spawn_times++;
	leave_critical();
  return 0;
}

static int do_kill(pid_t pid)
{
	pcb_t* task ;
	node_t*	the_task;
	lock_t *l;
	int i;
	enter_critical();
  /* TODO */
	 node_t *the_queue;
	if(pcb[pid].status == READY || pcb[pid].status == FIRST_TIME){
		the_queue = &ready_queue;
	}
	else if(pcb[pid].status == SLEEPING){
		the_queue = &sleep_wait_queue;
	}
	else if(pcb[pid].status == BLOCKED){
		the_queue = pcb[pid].wait_in;
		//print_hex(10, 24, current_running->entry_point);
	}
	else{
		return -1;
	}
  	 task = (pcb_t *)dequeue(&(pcb[pid].wait_queue));
	while(task != NULL){
		if(task->status == BLOCKED){
			unblock(task);
			//print_hex(10, 24, current_running->entry_point);
		}
		task = (pcb_t *)dequeue(&(pcb[pid].wait_queue));		
	}
	if(the_queue != NULL){
		while(the_queue != (node_t*)(&pcb[pid])){
			the_queue = the_queue -> next;
		}
	dequeue(the_queue->prev);
	}
	for(i = 0;i < pcb[pid].lock_num; i++){
		leave_critical();
		//print_hex(10, 24, current_running->entry_point);
		lock_release( (lock_t*) (pcb[pid].held_lock[i]));
		enter_critical();
	}
  pcb[pid].status = KILLED; 
  leave_critical();
  return -1;
}

static int do_wait(pid_t pid)
{
	enter_critical();
  //(void) pid;
	//printf(10,1, "  I'll wait in      (%d)        ", pid);
	//print_hex(10, 24, &(pcb[pid].wait_queue));
  block(&(pcb[pid].wait_queue));
  /* TODO */
  leave_critical();
  return -1;
}

static int	do_lock_acquire(mutex_t l){
	//print_hex(10, 24, l);
	current_running->held_lock[	current_running->lock_num++] = (void*) (&mux[l]);
	return lock_acquire(&mux[l]);
}

static void	do_lock_init(mutex_t l){
	//print_hex(10, 24, l);
	lock_init(&mux[l]);
}

static void	do_lock_release(mutex_t l){
	int i;
	int j;
	enter_critical();
	for( i=0; i<current_running->lock_num;i++){
		if(current_running->held_lock[i] == (void*)&mux[l]){
			for( j = i+1;j<current_running->lock_num;j++,i++){
				current_running->held_lock[i] = current_running->held_lock[j];
			}
			current_running->held_lock[i] = NULL;
			break;
		}
	}
	current_running->lock_num--;
	leave_critical();
	lock_release( &mux[l]);
}
