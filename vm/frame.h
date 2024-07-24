#ifndef VM_FRAME_H
#define VM_FRAME_H


void frame_init();
void frame_allocate(void *kpage);
void frame_map_page(uint32_t *pd, void *kpage, void *upage);


#endif