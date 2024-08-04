#ifndef VM_SWAP_H
#define VM_SWAP_H


#include "vm/sup-page-table.h"
void swap_init();
void swap_read(block_sector_t sector, void *kpage);
block_sector_t swap_write(void *upage);


#endif
