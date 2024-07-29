#include <stdbool.h>
#include "filesys/file.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include "vm/sup-page-table.h"

unsigned page_hash (const struct hash_elem *e, void *aux) {
    struct spte *spte = hash_entry(e, struct spte, hash_elem);
    hash_bytes(&spte->upage, sizeof(spte->upage));
}

bool page_hash_less (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
    struct spte *a_spte = hash_entry(a, struct spte, hash_elem);
    struct spte *b_spte = hash_entry(b, struct spte, hash_elem);

    return a_spte->upage < b_spte->upage; 
}

static void spte_destory (struct hash_elem *e, void *aux) {
    free(hash_entry(e, struct spte, hash_elem));
}

struct spt *
spt_create() {
    struct spt *spt = malloc(sizeof(struct spt));
    hash_init(&spt->spt, page_hash, page_hash_less, NULL);

    return spt;
}

void
sup_page_set_zero_page_lazy(struct spt *spt, void *upage, bool writable) {
    struct spte *spte = malloc(sizeof(struct spte));
    spte->upage = upage;
    spte->is_swapped = false;
    spte->fri = NULL;
    spte->sri = NULL;
    spte->writable = writable;
    hash_insert(&spt->spt, &spte->hash_elem);
}

void
sup_page_set_page_lazy(struct spt *spt, void *upage, bool writable, struct file_read_info *fri) {
    struct spte *spte = malloc(sizeof(struct spte));
    spte->upage = upage;
    spte->is_swapped = false;
    spte->fri = fri;
    spte->sri = NULL;
    spte->writable = writable;
    hash_insert(&spt->spt, &spte->hash_elem);
}

struct spte *
sup_page_get_page(struct spt *spt, void *addr) {
    void *upage = ((uint32_t)addr & ~PGMASK);
    struct spte spte;
    spte.upage = upage;
    struct hash_elem *elem = hash_find(spt, &spte.hash_elem);
    return elem != NULL ? hash_entry(elem, struct spte, hash_elem) : NULL;
}

void
sup_page_clear_page(struct spt *spt , struct spte *spte) {
    hash_delete(&spt->spt, &spte->hash_elem);
    free(spte);
}

void
spt_destory(struct spt *spt) {
    hash_destroy(&spt->spt, spte_destory);
}
