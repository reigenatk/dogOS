/**
 * @file kmalloc.h
 * @author Richard Ma
 * @brief kmalloc
 * @version 0.1
 * @date 2022-06-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef KMALLOC_H
#define KMALLOC_H

#include "../types.h"

#define MEGA_BYTE 	0x00100000
#define SLAB_SIZE 	0x00000200 				///< SLAB_SIZE = 512 bytes
#define KMEM_POOL 	0x00C00000				///< memory pool size = 12MB
#define SLAB_NUM	(KMEM_POOL/SLAB_SIZE)	///< total number of slabs in memory pool = 8192
#define INFO_SIZE 	sizeof(malloc_info_t)

typedef struct malloc_info{
	int start_addr;	///< the starting address of space
	int size;		///< number of slabs;
	char status; 	///< 0 for free, 1 for allocated
	struct memory_list *link;
} malloc_info_t;

typedef struct memory_list {
	char status;	///< 0 for not used;
	int size;
	struct memory_list *prev;		///< addr to the previous linked memory
	struct malloc_info *info;		///< malloc info
	struct memory_list *next;		///< pointer to the next item, NULL for last item
} memory_list_t;

void kmalloc_init();

void* kmalloc(size_t size);

void kfree(void* mem);

void* malloc(size_t size);

void* calloc(size_t count, size_t size);

void* free(void* mem);

int get_free_page();


/**
 * @brief All this does is give us just the right amount of pages so we can fit all the necessary stuff in
 * 
 * @param x 
 * @param y 
 * @return int 
 */
int ceiling_division(int x, int y);



#endif