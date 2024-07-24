#ifndef VM_SWAP_H
#define VM_SWAP_H


#include "vm/sup-page-table.h"
void swap_init();
void swap_in(uint32_t *pd, struct spt *spt, block_sector_t sector, void *kpage);
block_sector_t swap_out(void *upage);


#endif
