#include "common.h"
#include "mbox.h"
#include "sync.h"
#include "scheduler.h"

typedef enum {
	USED,
	UNUSED
} mbox_status;

typedef struct
{
	/* TODO */
	int data;
	uint32_t length;
} Message;

typedef struct
{
	/* TODO */
	char name[MBOX_NAME_LENGTH];
	Message buf[MAX_MBOX_LENGTH];
	uint32_t	message_num;
	mbox_t	ID;
	mbox_status status;
	uint32_t	user_num;
	semaphore_t	full_num;	//for comsumer
	semaphore_t empty_num;	//for producter
	lock_t	mutex;
} MessageBox;


static MessageBox MessageBoxen[MAX_MBOXEN];
lock_t BoxLock;

/* Perform any system-startup
 * initialization for the message
 * boxes.
 */
void init_mbox(void)
{
	/* TODO */
	int	i;
	for(i = 0;i < MAX_MBOXEN;i++){
		MessageBoxen[i].status = UNUSED;
		MessageBoxen[i].ID = i;
	}
}

/* Opens the mailbox named 'name', or
 * creates a new message box if it
 * doesn't already exist.
 * A message box is a bounded buffer
 * which holds up to MAX_MBOX_LENGTH items.
 * If it fails because the message
 * box table is full, it will return -1.
 * Otherwise, it returns a message box
 * id.
 */
mbox_t do_mbox_open(const char *name)
{
  //(void)name;
	/* TODO */
	enter_critical();
	int	length,i;
	for(i = 0;i < MAX_MBOXEN;i++){
		if(MessageBoxen[i].status == UNUSED) continue;
		else if(same_string(name,MessageBoxen[i].name)) {
			//MessageBoxen[i].user_num++;
			leave_critical();
			return	MessageBoxen[i].ID;
		}
	}
	//not exist
	for(i = 0;i < MAX_MBOXEN;i++){
		if(MessageBoxen[i].status == UNUSED){
			MessageBoxen[i].status = USED;
			bcopy(name,MessageBoxen[i].name,strlen(name));
			MessageBoxen[i].user_num = 1;
			MessageBoxen[i].message_num = 0;
			semaphore_init(&(MessageBoxen[i].full_num),0);
			semaphore_init(&(MessageBoxen[i].empty_num),MAX_MBOX_LENGTH);
			lock_init(&(MessageBoxen[i].mutex));
			leave_critical();
			return	MessageBoxen[i].ID;
		}
	}
	leave_critical();
	return	-1;
}

/* Closes a message box
 */
void do_mbox_close(mbox_t mbox)
{
  (void)mbox;
	/* TODO */
	enter_critical();
	MessageBoxen[(int)mbox].status = UNUSED;
	leave_critical();
}

/* Determine if the given
 * message box is full.
 * Equivalently, determine
 * if sending to this mbox
 * would cause a process
 * to block.
 *FULL return 1; else return 0 
 */
int do_mbox_is_full(mbox_t mbox)
{
  (void)mbox;
	/* TODO */
	return	MessageBoxen[(int)mbox].message_num == MAX_MBOX_LENGTH;
}

/* Enqueues a message onto
 * a message box.  If the
 * message box is full, the
 * process will block until
 * it can add the item.
 * You may assume that the
 * message box ID has been
 * properly opened before this
 * call.
 * The message is 'nbytes' bytes
 * starting at offset 'msg'
 */
void do_mbox_send(mbox_t mbox, int *msg, int nbytes)
{
  //(void)mbox;
  //(void)msg;
  //(void)nbytes;

  /* TODO */
	enter_critical();
  //int	i;
  MessageBox	*BOX ;
  Message*		MES ;
	
	BOX = &(MessageBoxen[(int)mbox]);
	MES = &(BOX->buf[BOX->message_num]);
leave_critical();
  semaphore_down(&(BOX->empty_num));
  lock_acquire(&(BOX->mutex));
	enter_critical();
	//BOX = &(MessageBoxen[(int)mbox]);
	MES->data=(*msg); 
	BOX->message_num++;
	//BOX->message_num++;
  MES->length = nbytes;
	  //MES->data = (*msg); 
	leave_critical();
  lock_release(&(BOX->mutex));
  semaphore_up(&(BOX->full_num));
}

/* Receives a message from the
 * specified message box.  If
 * empty, the process will block
 * until it can remove an item.
 * You may assume that the
 * message box has been properly
 * opened before this call.
 * The message is copied into
 * 'msg'.  No more than
 * 'nbytes' bytes will by copied
 * into this buffer; longer
 * messages will be truncated.
 */
void do_mbox_recv(mbox_t mbox, int *msg, int nbytes)
{
  //(void)mbox;
  //(void)msg;
  //(void)nbytes;
  /* TODO */
	enter_critical();
  int	i;
  MessageBox	*BOX ;
  Message		*MES ;
	//print_hex(10, 24, (uint32_t)(BOX->full_num.value));
	
BOX = &MessageBoxen[(int)mbox];
BOX->message_num--;
MES = &(BOX->buf[BOX->message_num]);
leave_critical();
  semaphore_down(&(BOX->full_num));
  lock_acquire(&(BOX->mutex));
	
	enter_critical();
	//BOX = &MessageBoxen[(int)mbox];
	//BOX->message_num--;
	//MES = &(BOX->buf[BOX->message_num]);
	  (*msg)  = MES->data; 
	leave_critical();
  lock_release(&(BOX->mutex));
  semaphore_up(&(BOX->empty_num));
}


