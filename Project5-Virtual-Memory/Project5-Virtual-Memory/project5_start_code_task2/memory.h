/* Author(s): <Your name here>
 * Defines the memory manager for the kernel.
*/

#ifndef MEMORY_H
#define MEMORY_H

enum {
  /* physical page facts */
  PAGE_SIZE = 4096,
  PAGE_N_ENTRIES = (PAGE_SIZE / sizeof(uint32_t)),

  // Global bit
  PE_G = (0x40 >> 6),
  // Valid bit
  PE_V = (0x80 >> 6),
  // Writable bit
  PE_D = (0x100 >> 6),
  // Uncache bit
  PE_UC = (0x400 >> 6),

  /* Constants to simulate a very small physical memory. */
  PAGEABLE_PAGES = 3,
};

/* TODO: Structure of an entry in the page map */
typedef struct {
    // design here
	uint32_t	entrylo_0;
	uint32_t	entrylo_1;
	uint32_t	entryhi;
	uint32_t	daddr;
	
} PTE;
//V、R、M、（T）、对应的虚拟地址、内容所在的物理地址、保存区, pid,flag

typedef struct {
    // design here
	int			num;
	uint32_t	paddr;			
	uint32_t	vaddr;		//映射到内核
	uint8_t		pin;
	uint8_t		used;
	struct page_map_entry_t *next;
	PTE		*pte;
} page_map_entry_t;

page_map_entry_t *head;
page_map_entry_t *tail;

/* Prototypes */

/* Return the physical address of the i-th page */
uint32_t* page_addr(int i);

/* Allocate a page.  If necessary, swap a page out.
 * On success, return the index of the page in the page map.  On
 * failure, abort.  BUG: pages are not made free when a process
 * exits.
 */
int page_alloc(int pinned,PTE *pte);

/* init page_map */
uint32_t init_memory(void);

/* Set up a page directory and page table for the given process. Fill in
 * any necessary information in the pcb. 
 */
uint32_t setup_page_table(int pid);

// other functions defined here
//

#endif /* !MEMORY_H */
