/* Author(s): <Your name here>
 * Implementation of the memory manager for the kernel.
 */

#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "memory.h"
#include "printf.h"
#include "scheduler.h"
#include "util.h"

#define MEM_START 0xa0908000
 node_t	busy_page;
 node_t	spare_page;
 int	page_fault_num;
 int	TLB_miss_num;
/* Static global variables */
// Keep track of all pages: their vaddr, status, and other properties
static page_map_entry_t page_map[ PAGEABLE_PAGES ];
page_map_entry_t *head;
page_map_entry_t *tail;
// other global variables...
/*static lock_t page_fault_lock;*/

/* TODO: Returns virtual address of page number i */
uint32_t page_vaddr( int i ) {
    return page_map[i].vaddr;
}

/* TODO: Returns  physical address (in kernel) of page number i */
uint32_t page_paddr( int i ) {
    return page_map[i].paddr;
}

/* get the physical address from virtual address (in kernel) */
uint32_t va2pa( uint32_t va ) {
    return (uint32_t) va - 0xa0000000;
}

/* get the virtual address (in kernel) from physical address */
uint32_t pa2va( uint32_t pa ) {
    return (uint32_t) pa + 0xa0000000;
}


// TODO: insert page table entry to page table
void insert_page_table_entry( uint32_t *table, uint32_t vaddr, uint32_t paddr,
                              uint32_t flag, uint32_t pid ) {
    // insert entry
	PTE* T = (PTE*)table;
	uint32_t	entrylo;
	while(1){
		if((T->entryhi)>>13 == vaddr){
			entrylo = (paddr >> 12) << 6;
			if(flag == 0){
				T->entrylo_0 |=  (entrylo | PE_V);
			}
			else if(flag == 1){
				T->entrylo_1 |=  (entrylo | PE_V);
			}
			break;
		}
		else	T++;
	}
	tlb_flush(T->entryhi);
    // tlb flush
}

/* TODO: Allocate a page. Return page index in the page_map directory.
 *
 * Marks page as pinned if pinned == TRUE.
 * Swap out a page if no space is available.
 */
int page_alloc( int pinned ,PTE *pte) {
 
    // code here
	page_map_entry_t *chose;
	int	i;
	PTE	*P;
	for(i = 0; i < PAGEABLE_PAGES; i++){
		if(page_map[i].used == 0){
			page_map[i].used = 1;
			page_map[i].pin = pinned;
			page_map[i].pte = pte;
			tail->next = &(page_map[i]);
			tail = &(page_map[i]);
			tail->next = NULL;
				
			return i;
		}
	}

	while(1){
		if(head->pin == 0){
			P = head->pte;
			tlb_flush(P->entryhi);
			if((P->entrylo_0 >> 6) == (page_paddr(head->num)>>12)){
				bcopy((char*)page_vaddr(head->num),(char*)P->daddr,PAGE_SIZE);
				P->entrylo_0 &= (~PE_V) ;
				
			}
			else{
				bcopy((char*)page_vaddr(head->num),(char*)(P->daddr+PAGE_SIZE),PAGE_SIZE);
				P->entrylo_1 &= (~PE_V) ;
			}
			head -> pin = pinned;
			head -> pte = pte;
			tail -> next = head;
			head = head -> next;
			tail = tail->next;
			tail->next = NULL;
			return tail->num;
		}
		tail -> next = head;
		head = head -> next;
		tail = tail->next;
		tail->next = NULL;
		
		
	}
    //ASSERT( i < PAGEABLE_PAGES );

}

/* TODO: 
 * This method is only called once by _start() in kernel.c
 */
uint32_t init_memory( void ) {
    // initialize all pageable pages to a default state
	uint32_t begin = MEM_START;
	queue_init( &busy_page );
	queue_init( &spare_page );
	int	i;
	for(i = 0;i < PAGEABLE_PAGES;i++,begin+=PAGE_SIZE){
		page_map[i].num = i;
		page_map[i].vaddr = begin;
		page_map[i].paddr = va2pa(begin);
		page_map[i].used = 0;
		page_map[i].pin = 0;
	}
	head = page_map;
	tail = page_map;
	page_fault_num = 0;
	TLB_miss_num = 0;
	return 0;
}

void create_pte(uint32_t vaddr, int pid,PTE* pte,uint32_t daddr) {
	pte->entryhi = (vaddr<<13)|pid;
	pte->entrylo_0 = PE_G   | PE_D | PE_UC;
	pte->entrylo_1 = PE_G   | PE_D | PE_UC;
	pte->daddr = daddr;
	tlb_flush(pte->entryhi);
    return;
}
/* TODO: 
 * 
 */
uint32_t setup_page_table( int pid ) {
	if(pid == 0)	return 0;
	
    uint32_t page_table;
	int	num = pcb[pid].size / PAGE_SIZE;
	int vaddr = pcb[pid].entry_point;
	int	i;
	int	loc = pcb[pid].loc;
	PTE* pte;
	int	page_num1,page_num2;
	uint32_t		stack_vaddr;
	uint32_t	*test;
    // alloc page for page table 
	page_table = page_alloc( 1,0);
	pte = (PTE*)page_vaddr(page_table);
    // initialize PTE and insert several entries into page tables using insert_page_table_entry
    for(i = 0; i<= num;i+=2){
		create_pte(vaddr>>13,pid,pte,loc);
		loc+=PAGE_SIZE;
		pte++;
		vaddr+=(PAGE_SIZE<<1);
		loc+=PAGE_SIZE;
	}	
    return page_table;
}

void do_page_fault(uint32_t vaddr, PTE* pte,int flag,int pid) {
	uint32_t page_table;
	int	page_num;
	page_num = page_alloc( 0,pte);
	page_table = pcb[pid].page_table;
	if(flag == 1)	bcopy((char*)(pte->daddr+PAGE_SIZE),(char*)page_vaddr(page_num),PAGE_SIZE);
	else		bcopy((char*)(pte->daddr),(char*)page_vaddr(page_num),PAGE_SIZE);
	insert_page_table_entry(page_vaddr(page_table),vaddr,page_paddr(page_num),flag,pid);
	page_fault_num++;
	printf( 20, 1, "PAGE FAULT :" );
	printf( 21, 1, "%d",page_fault_num);
}

uint32_t do_tlb_miss(uint32_t vaddr, int pid) {
	enter_critical();
	//print_hex(10, 24, vaddr);
	//print_hex(20, 24, pid);
	TLB_miss_num ++;
	printf( 22, 1, "TLB MISS :" );
	printf( 23, 1, "%d",TLB_miss_num);
	PTE* T = (PTE*)page_vaddr(pcb[pid].page_table);
	while(1){
		//print_hex(30, 24, ((T->entryhi)>>13));
		//print_hex(40, 24, ((T->entryhi)&0x0111));
		if(((T->entryhi)>>13) == (vaddr>>1) && ((T->entryhi)&0x0fff) == pid){
			if((vaddr & 0x1)==0 && ((T->entrylo_0)&PE_V) == 0){
				do_page_fault(vaddr>>1,T,0,pid);
			}
			else if((vaddr & 0x1)==1 && ((T->entrylo_1)&PE_V) == 0){
				do_page_fault(vaddr>>1,T,1,pid);
			}
			return (uint32_t)T;
		}
		else	T++;
	}
}
