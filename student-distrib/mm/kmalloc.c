#include "kmalloc.h"
#include "../libc/sys/types.h"
#include "../paging.h"
#include "../errno.h"

static struct memory_list mem_info[SLAB_NUM];

// two linked lists of free and allocated memory chunks
// which are really just at any given point, pointers to the above array mem_info
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
    // GIVE THE free list all the slabs at the beginning (because nothing is allocated) and 
    free_list = &(mem_info[0]);
	free_list->status = 1;
	free_list->size = SLAB_NUM;
	free_list->prev = NULL;

    // important note, the malloc info struct is always placed right at the beginning of the allocated memory chunk.
    // this is consistent with the heap chals I did a while back from sigpwny! Obviously fields aren't exactly the same but anyways...
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
    
    // find a memory chunk, temp, which has equal or greater amount of contiguous slabs that we have requested
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

        if (alloc_list == NULL) {
            // if allocating all 12MB immediately lol (rare case, use has to request exactly 12MB)
            // then the entire chunk goes into alloc and free becomes empty
            alloc_list = temp;
            free_list = NULL;
        }
        else {
            temp2 = alloc_list;

            // find end of the allocated list
            while (temp2->next) {
                temp2 = temp2->next;
            }

            // insert new list at end of the 'allocated' LL
            temp2->next = temp;
            temp->prev = temp2;
            temp->next = 0;
            temp->info->status = 1; // mark info struct as allocated  
            // no need to mark temp status since its should already be 1 from before (either via initialization or a split)
        }

    }
    else {
        // we have to split this group of slabs in two (one is free, one is alloc)
        // say that temp has y slabs and we need x where y > x, then its broken up into 2 chunks: x alloc'd slabs, and (y-x) free slabs.
        // temp is the alloc'd one (but starts off as empty) and temp2 is the free one
        ret = temp->info->start_addr;

        // prepare info entry for new slab (this is the free one)
        malloc_info_t* new_entry_info = (malloc_info_t*)(ret + min_slabs * SLAB_SIZE);
        
        // find somewhere to put our allocated stuff
        int i;
        for (i = 0; i < SLAB_NUM; i++) {
            if (mem_info[i].status == 0) {
                // temp2 is the free one now
                temp2 = &mem_info[i];
                temp2->status = 1; // mark the chunk as used
                break;
            }
        }
        if (i == SLAB_NUM) {
            errno = -ENOMEM;
            return 0;
        }
        // set the properties on the free block now
        temp2->size = temp->size - min_slabs;
        
        // set the info on that block as well
        new_entry_info->link = temp2;
        new_entry_info->size = temp2->size;
        new_entry_info->start_addr = temp->info->start_addr + SLAB_SIZE * min_slabs;
        new_entry_info->status = 0; // free
        temp2->info = new_entry_info;

        // insert temp2 into free LL (and remove temp from it)
        if (temp->prev) {
            temp->prev->next = temp2;
        }
        if (temp->next) {
            temp->next->prev = temp2;
        }

        // adjust temp
        temp->size = min_slabs;
        temp->info->size = min_slabs;
        temp->info->status = 1; // allocated

        // put temp into alloc list
        temp2 = alloc_list;

        if (temp2 == NULL) {
            // that is, the first one to be in the alloc list
            alloc_list = temp;
            temp->prev = NULL;
            temp->next = NULL;
        }
        else {
            // insert this list into allocated list
            while (temp2->next) {
                temp2 = temp2->next;
            }
            temp2->next = temp;
            temp->prev = temp2;
            temp->next = NULL;
        }

    }
    return (void*)(ret + INFO_SIZE);
}

int kfree(void* mem) {
    malloc_info_t *info_about_mem_chunk = (malloc_info_t *)((uint32_t) mem - INFO_SIZE);
    if (info_about_mem_chunk->status != 1) {
        // then its not valid
        return -EINVAL;
    }

    // the list entity
    memory_list_t *memory_chunk = info_about_mem_chunk->link;

    // use this to loop thru free list
    memory_list_t *temp = free_list;

    // OK idk why they did this, I disagree with their implementation. Should be temp->size * SLAB_SIZE no? Cuz you wanna check if 
    // current slab is right before the one we're about to free. And doing offset makes no sense since that's the size of the freed chunk?
    int offset = info_about_mem_chunk->size * SLAB_SIZE;
    if (temp) {
        // if the freelist isnt empty, first find the chunk that is right before it
        while (temp->next) {
            if (temp->info->start_addr + temp->size * SLAB_SIZE == info_about_mem_chunk->start_addr) {
                break;
            }
            temp = temp->next;
        }

        // ok now here they break it into cases. Again idk why we can't jus do this: add the chunk we're freeing's size to the previous one
        // and then set it to zero. done?
        temp->size += info_about_mem_chunk->size * SLAB_SIZE;
        temp->info->size = temp->size;
        memset(memory_chunk, 0, info_about_mem_chunk->size * SLAB_SIZE);
        
    }
    else {
        // free list is empty
        free_list = temp;
        temp->prev = NULL;
        temp->next = NULL;
        temp->info->status = 0;
    }
    return 0;
}

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