#ifndef VM_SUP_PAGE_TABLE_H
#define VM_SUP_PAGE_TABLE_H

#include <hash.h>
#include "devices/block.h"

struct spt {
    struct hash spt;
};

struct spte {
    void *upage;
    bool is_swapped;
    struct file_read_info *fri;
    struct swap_read_info *sri;
    struct hash_elem hash_elem;
    bool writable;
};

struct file_read_info {
    struct file *file;
    int read_bytes;
};

struct swap_read_info {
    block_sector_t sector;
};

struct spt *spt_create();
void sup_page_set_zero_page_lazy(struct spt *spt, void *upage, bool writable);
void sup_page_set_page_lazy(struct spt *spt, void *upage, bool writable, struct file_read_info *fri);
struct spte * sup_page_get_page(struct spt *spt, void *addr);
void sup_page_clear_page(struct spt *spt , struct spte *spte);
void spt_destory(struct spt *spt);


#endif