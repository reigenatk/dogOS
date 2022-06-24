#include "kmalloc.h"
#include "../libc/sys/types.h"
#include "../paging.h"
#include "../errno.h"

static struct memory_list mem_info[SLAB_NUM];
static struct memory_list *free_list;
static struct memory_list *alloc_list;

static char kmalloc_initialized = 0;								// indicate if memory pool is initialized
static int 	start_addr;
static int 	cur_addr = 0;							// indicate virtual addr for next allocated starting page addr. We wanna allocate the memory contiguously (virtually)


void kmalloc_init() {
    int i; 			// iterator
	int addr = 0;	// request addr from page;

	for (i = 1; i < SLAB_NUM; i++) {
		mem_info[i].status = 0;
		mem_info[i].size = 0;
		mem_info[i].prev = NULL;
		mem_info[i].info = NULL;
		mem_info[i].next = NULL;
	}


	// REQUESTE 3 4MB PAGES- the slab allocator will start returning memory from here!
	page_alloc_4MB(&addr);
	start_addr = addr;
	cur_addr = addr + (4 * MEGA_BYTE);
	page_dir_add_4MB_entry(addr, addr, PRESENT_BIT | READ_WRITE_BIT |
						PAGE_SIZE_BIT |
						GLOBAL_BIT);

    // allocate other 2 4MB pages
	get_free_page();
	get_free_page();

    // initialize the info for the first free list 
    ((malloc_info_t*) start_addr)->start_addr = start_addr;
    ((malloc_info_t*) start_addr)->status = 0; // not alocated
    ((malloc_info_t*) start_addr)->size = SLAB_NUM;

    // initialize the starting two lists
    free_list = &(mem_info[0]);
	free_list->status = 1;
	free_list->size = SLAB_NUM;
	free_list->prev = NULL;
	free_list->info = ((malloc_info_t*)start_addr);
	((malloc_info_t*)start_addr)->link = free_list;
	free_list->next = NULL;

	alloc_list = NULL;
	// alloc_list->status = 1;
	// alloc_list->size = 0;
	// alloc_list->prev = NULL;
	// alloc_list->info = NULL;
	// alloc_list->next = NULL;

    // mark as initialized
	kmalloc_initialized = 1;
}

void* kmalloc(size_t size) {
    int ret;
    if (kmalloc_initialized == 0) {
        kmalloc_init();
    }
    // find min number of slabs we need to use to fulfill the size request
    // note how we include the INFO_SIZE, that's cuz we put the slab info at the beginning of the first 4MB page
    // and as you will see later, we're gonna add an offset when we return to account for this
    int min_slabs = ceiling_division(size + INFO_SIZE, SLAB_SIZE);

    // find the first memory list (inside the free linked list) with enough slabs (temp)
    memory_list_t* temp = free_list;
    
    while (temp != NULL) {
        if (temp->size >= min_slabs) {
            break;
        }
        // otherwise keep looking
        temp = temp->next;
    }

    memory_list_t* temp2;
    // if the number of free slabs is exactly what we need, remove from free list and put into alloc list
    if (min_slabs == temp->size) {
        // first grab the address
        ret = temp->info->start_addr;

        // we are gonna remove this memory_list from the linked list
        if (temp->prev != NULL)
			temp->prev->next = temp->next;
		if (temp->next != NULL)
			temp->next->prev = temp->prev;

        temp2 = alloc_list;

        // insert this list into allocated list
        while (temp2->next) {
            temp2 = temp2->next;
        }

        // insert new list at end of the 'allocated' LL
        temp2->next = temp;
        temp->prev = temp2;
        temp->next = 0;
        temp->info->status = 1; // mark info struct as allocated  
    }
    else {
        // we have to split this group of slabs in two (one is free, one is alloc)
        ret = temp->info->start_addr;

        // prepare info entry for new slab (this is the free one)
        memory_list_t* new_entry_info = (memory_list_t*)(ret + min_slabs * SLAB_SIZE);
        
        // find somewhere to put our allocated stuff
        int i;
        for (i = 0; i < SLAB_NUM; i++) {
            if (mem_info[i].status == 0) {
                // temp2 is the free one now
                temp2 = &mem_info[i];
                temp2->status = 1;
                break;
            }
        }
        if (i == SLAB_NUM) {
            errno = -ENOMEM;
            return 0;
        }

        temp2->size = temp->size - min_slabs;
    }
    return (void*)(ret + INFO_SIZE);
}

void kfree(void* mem);

void* malloc(size_t size);

void* calloc(size_t count, size_t size);

void* free(void* mem);

int get_free_page() {

	if (kmalloc_initialized == 0) {
		return -ENOBUFS;
	}

	if (cur_addr - start_addr >= (12 * MEGA_BYTE)) {
		return -ENOBUFS;
	}

	int addr = 0;
	page_alloc_4MB(&addr);
	page_dir_add_4MB_entry(cur_addr, addr, PRESENT_BIT | READ_WRITE_BIT |
						PAGE_SIZE_BIT |
						GLOBAL_BIT);
	cur_addr += (4 * MEGA_BYTE);

	return 0;
}

int ceiling_division(int x, int y) {
	if ((x % y) != 0)
		return (x/y) + 1;
	else
		return x/y;
}