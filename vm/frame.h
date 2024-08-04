#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <hash.h>
#include <list.h>

/* 커널 실행 코드를 제외한 (1MB) 물리메모리를 관리하기 위한 구조체 */
static struct frame {
    void *kpage;
    uint32_t *pd;
    struct spt *spt;
    void *upage;
    struct hash_elem frame_elem;
    struct list_elem eviction_elem;
};


void frame_init();
void frame_allocate(void *kpage);
void frame_free(void *kpage);
void frame_map_page(uint32_t *pd, struct spt *spt, void *kpage, void *upage);
struct frame *frame_evict();

#endif