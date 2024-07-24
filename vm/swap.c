#include "devices/block.h"
#include <bitmap.h>
#include "devices/block.h"
#include "threads/vaddr.h"

/* swap은 페이지 단위로 swap in/out */

/* Partition that contains the swap device. */
struct block *swap_device;
struct bitmap *swap_table;
static int SECTOR_UNIT_SIZE = PGSIZE / BLOCK_SECTOR_SIZE;

void
swap_init() {
    swap_device = block_get_role(BLOCK_SWAP);
    swap_table = bitmap_create(block_size(swap_device));
}

void
swap_in(uint32_t *pd, struct spt *spt, block_sector_t sector, void *kpage) {
    void *read_buffer = kpage;
    for(int i=0 ; i<SECTOR_UNIT_SIZE ; i++) {
        block_read(swap_device, sector+ i, read_buffer);
        read_buffer += BLOCK_SECTOR_SIZE;
    }
}

block_sector_t
swap_out(void *upage) {
    size_t sector_idx = bitmap_scan_and_flip(swap_table, 0, SECTOR_UNIT_SIZE, false);
    void *write_buffer = upage;
    for(int i=0 ; i<SECTOR_UNIT_SIZE ; i++) {
        block_write(swap_device, sector_idx + i, write_buffer);
        write_buffer += BLOCK_SECTOR_SIZE;
    }
}

