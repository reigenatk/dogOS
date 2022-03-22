#ifndef PAGING_H
#define PAGING_H

#define NUMENTRIES 1024
#define FOURKB 4096
#define FOURMB 4194304

#include "types.h"
#include "lib.h"


typedef struct page_directory_entry {
  union 
  {
    uint32_t entry;
    struct fourkb {
      uint32_t page_table_base_address : 20;
      uint32_t avail                   : 4;
      uint32_t global_page             : 1;
      uint32_t page_size               : 1;
      uint32_t reserved                : 1;
      uint32_t accessed                : 1;
      uint32_t cache_disabled          : 1;
      uint32_t write_through           : 1;
      uint32_t user_supervisor         : 1;
      uint32_t read_write              : 1;
      uint8_t  present                 : 1;
    } fourkb __attribute__ ((packed));
    struct fourmb {
      uint32_t page_base_address           : 10;
      uint32_t reserved                    : 9;
      uint32_t page_table_attribute_index  : 1;
      uint32_t available                   : 4;
      uint32_t global_page                 : 1;
      uint32_t page_size                   : 1;
      uint32_t dirty                       : 1;
      uint32_t accessed                    : 1;
      uint32_t cache_disabled              : 1;
      uint32_t write_through               : 1;
      uint32_t user_supervisor             : 1;
      uint32_t read_write                  : 1;
      uint8_t  present                     : 1;
    } fourmb __attribute__ ((packed));
  };
} page_directory_entry;

typedef struct page_table_entry {
  union 
  {
    uint32_t entry;
    struct {
      uint32_t page_base_address           : 20;
      uint32_t avail                       : 4;
      uint32_t page_table_attribute_index  : 1;
      uint32_t dirty                       : 1;
      uint32_t accessed                    : 1;
      uint32_t cache_disabled              : 1;
      uint32_t write_through               : 1;
      uint32_t user_supervisor             : 1;
      uint32_t read_write                  : 1;
      uint8_t  present                     : 1;
    } __attribute__ ((packed));
  };
} page_table_entry;

// typedef struct page_directory_4mb_entry {
//   union 
//   {
//     uint32_t entry;
//     struct {
//       uint32_t page_base_address           : 10;
//       uint32_t reserved                    : 9;
//       uint32_t page_table_attribute_index  : 1;
//       uint32_t available                   : 4;
//       uint32_t global_page                 : 1;
//       uint32_t page_size                   : 1;
//       uint32_t dirty                       : 1;
//       uint32_t accessed                    : 1;
//       uint32_t cache_disabled              : 1;
//       uint32_t write_through               : 1;
//       uint32_t user_supervisor             : 1;
//       uint32_t read_write                  : 1;
//       uint8_t  present                     : 1;
//     } __attribute__ ((packed));
//   };
// } page_directory_4mb_entry;

void setup_paging();

// align page tables on 4KB boundaries
extern page_directory_entry page_directory[NUMENTRIES] __attribute__((aligned(FOURKB)));
extern page_table_entry page_table[NUMENTRIES] __attribute__((aligned(FOURKB)));

#endif
