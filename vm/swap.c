#include "devices/block.h"
#include <bitmap.h>
#include "devices/block.h"
#include "threads/vaddr.h"
#include "vm/swap.h"
#include "threads/synch.h"

/* swap은 페이지 단위로 swap in/out */

/* Partition that contains the swap device. */
static struct block *swap_device;
static struct bitmap *swap_table;
static int SECTOR_UNIT_SIZE = PGSIZE / BLOCK_SECTOR_SIZE;
static struct lock swap_table_lock;

void
swap_init() {
    swap_device = block_get_role(BLOCK_SWAP);
    swap_table = bitmap_create(block_size(swap_device));
    lock_init(&swap_table_lock);
} 

void
swap_read(block_sector_t sector, void *kpage) {
    lock_acquire(&swap_table_lock);
    void *read_buffer = kpage;
    for(int i=0 ; i<SECTOR_UNIT_SIZE ; i++) {
        block_read(swap_device, sector+ i, read_buffer);
        read_buffer += BLOCK_SECTOR_SIZE;
    }
    bitmap_set_multiple(swap_table, sector, SECTOR_UNIT_SIZE, false);
    lock_release(&swap_table_lock);
}

block_sector_t
swap_write(void *upage) {
    lock_acquire(&swap_table_lock);
    size_t sector_idx = bitmap_scan_and_flip(swap_table, 0, SECTOR_UNIT_SIZE, false);
    void *write_buffer = upage;
    for(int i=0 ; i<SECTOR_UNIT_SIZE ; i++) {
        block_write(swap_device, sector_idx + i, write_buffer);
        write_buffer += BLOCK_SECTOR_SIZE;
    }
    lock_release(&swap_table_lock);
    
    return sector_idx;
}

