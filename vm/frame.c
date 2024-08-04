#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>
#include <hash.h>
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "vm/frame.h"

static struct hash frame_table;

/* frame eviction 전략 */
static struct list eviction_list;
static struct list_elem *eviction_ptr;

static struct lock ft_lock;

static struct list_elem * next_eviction_ptr(struct list_elem *elem);
static struct frame *find_frame(void *kpage);

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
    list_init(&eviction_list);
    lock_init(&ft_lock);
    eviction_ptr = list_tail(&eviction_list);
}

void 
frame_allocate(void *kpage) {
    lock_acquire(&ft_lock);
    struct frame *f = malloc(sizeof (struct frame));
    f->kpage = kpage;
    f->pd = NULL;
    f->upage = NULL;
    hash_insert(&frame_table, &f->frame_elem);
    list_insert(eviction_ptr, &f->eviction_elem);
    if(hash_size(&frame_table) == 1) {
        eviction_ptr = list_begin(&eviction_list);
    }
    lock_release(&ft_lock);
}

void 
frame_free(void *kpage) {
    if(find_frame(kpage) == NULL) {
        return;
    }
    lock_acquire(&ft_lock);
    struct frame f;
    f.kpage = kpage;
    struct hash_elem *delete_hash_elem = hash_delete(&frame_table, &f.frame_elem);
    struct frame *delete_f = hash_entry(delete_hash_elem, struct frame, frame_elem);
    if(&delete_f->eviction_elem == eviction_ptr) {
        eviction_ptr = next_eviction_ptr(eviction_ptr);
    }
    list_remove(&delete_f->eviction_elem);
    lock_release(&ft_lock);
}

/* FIXME: frame_allocate 이후에 frame_map_page가 안된 상태이면??? */
struct frame *
frame_evict() {
    lock_acquire(&ft_lock);
    struct list_elem *eviction_iter_ptr = eviction_ptr;
    struct list_elem *eviction_candidate_elem = NULL;

    do {
        struct frame *f = list_entry(eviction_iter_ptr, struct frame, eviction_elem);
        ASSERT(f->upage != NULL);
        if(pagedir_is_accessed(f->pd, f->upage)) {
            pagedir_set_accessed(f->pd, f->upage, false);
            eviction_iter_ptr = next_eviction_ptr(eviction_iter_ptr);
        } else {
            if(pagedir_is_dirty(f->pd, f->upage)) {
                if(eviction_candidate_elem == NULL) {
                    eviction_candidate_elem = &f->eviction_elem;
                }
                eviction_iter_ptr = next_eviction_ptr(eviction_iter_ptr);
                continue;
            }
            eviction_candidate_elem = &f->eviction_elem;
            eviction_iter_ptr = next_eviction_ptr(eviction_iter_ptr);
            break;
        
        }
    } while(eviction_iter_ptr != eviction_ptr);

    eviction_ptr = eviction_iter_ptr;
    struct frame *eviction_f;
    if(eviction_candidate_elem == NULL) {
        eviction_f = list_entry(eviction_ptr, struct frame, eviction_elem);
    } else {
        eviction_f = list_entry(eviction_candidate_elem, struct frame, eviction_elem);
    }

    frame_free(eviction_f->kpage);
    lock_release(&ft_lock);

    return eviction_f;
}

static struct list_elem *
next_eviction_ptr(struct list_elem *elem) {
    struct list_elem *next_eviction_ptr = list_next(elem);
    if(list_end(&eviction_list) == next_eviction_ptr) {
        next_eviction_ptr = list_begin(&eviction_list);
    }

    return next_eviction_ptr;
}

void
frame_map_page(uint32_t *pd, struct spt *spt, void *kpage, void *upage) {
    lock_acquire(&ft_lock);
    struct frame *f = find_frame(kpage);
    f->pd = pd;
    f->spt = spt;
    f->upage = upage;
    lock_release(&ft_lock);
}

static struct frame *
find_frame(void *kpage) {
    struct frame find_f;
    find_f.kpage = kpage;
    struct hash_elem *f_elem = hash_find(&frame_table, &find_f.frame_elem);
    if(f_elem == NULL) {
        return NULL;
    }
    return hash_entry(f_elem, struct frame, frame_elem);
}