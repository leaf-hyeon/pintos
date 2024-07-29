#ifndef VM_MMAP_H
#define VM_MMAP_H


struct mmap {
    void *page;
    int file_size;
};

int mmap_mapping(int fd, void *addr);
void mmap_unmap(int mapping);
int mmap_get_page_num(struct mmap *mmap);



#endif