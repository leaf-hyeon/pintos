#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>
#include <hash.h>
#include "threads/vaddr.h"

/*
user process가 할당된 frame을 관리하기 위한 frame_table
*/
static struct hash frame_table;

static struct frame {
    void *kpage;
    uint32_t *pd;
    void *upage;
    struct hash_elem frame_elem;
};


unsigned frame_hash (const struct hash_elem *e, void *aux) {
    struct frame *frame = hash_entry(e, struct frame, frame_elem);
    hash_bytes(&frame->kpage, sizeof(frame->kpage));
}

bool frame_hash_less (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct frame *a_frame = hash_entry(a, struct frame, frame_elem);
    struct frame *b_frame = hash_entry(b, struct frame, frame_elem);

    return a_frame->kpage < b_frame->kpage; 
}

void 
frame_init() {
    hash_init(&frame_table, frame_hash, frame_hash_less, NULL);
}

void 
frame_allocate(void *kpage) {
    struct frame *f = malloc(sizeof (struct frame));
    f->kpage = kpage;
    f->pd = NULL;
    f->upage = NULL;
    hash_insert(&frame_table, &f->frame_elem);
    // evict
}

void
frame_map_page(uint32_t *pd, void *kpage, void *upage) {
    struct frame find_f;
    find_f.kpage = kpage;

    struct hash_elem *f_elem = hash_find(&frame_table, &find_f.frame_elem);
    struct frame *f = hash_entry(f_elem, struct frame, frame_elem);
    f->pd = pd;
    f->upage = upage;
}