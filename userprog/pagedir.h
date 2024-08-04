#ifndef USERPROG_PAGEDIR_H
#define USERPROG_PAGEDIR_H

#include <stdbool.h>
#include <stdint.h>
#include "vm/sup-page-table.h"

uint32_t *pagedir_create (void);
void pagedir_destroy (uint32_t *pd);
bool pagedir_set_page (uint32_t *pd, struct spt *spt, void *upage, bool writable, struct file_read_info *fri);
void pagedir_delete_page(uint32_t *pd, struct spt *spt, void *upage);
void *pagedir_get_page (uint32_t *pd, const void *upage);
void pagedir_clear_page (uint32_t *pd, void *upage);
bool pagedir_is_dirty (uint32_t *pd, const void *upage);
void pagedir_set_dirty (uint32_t *pd, const void *upage, bool dirty);
bool pagedir_is_accessed (uint32_t *pd, const void *upage);
void pagedir_set_accessed (uint32_t *pd, const void *upage, bool accessed);
void pagedir_activate (uint32_t *pd);
void pagedir_page_in(uint32_t *pd, struct spt *spt, void *upage);
void pagedir_page_out(uint32_t *pd, struct spt *spt, void *upage);

#endif /* userprog/pagedir.h */
