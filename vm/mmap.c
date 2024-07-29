#include "vm/mmap.h"
#include "threads/vaddr.h"
#include <round.h>
#include "threads/thread.h"
#include "filesys/file.h"
#include "vm/sup-page-table.h"

int 
mmap_mapping(int fd, void *addr) {
    struct thread *t = thread_current();
    struct file **fdt = thread_current()->fdt;
    struct file *file = fdt[fd];
    int f_size = file_length(file);
    int page_num = DIV_ROUND_UP(f_size, PGSIZE);

    void *next_page = addr;
    int total_read_bytes = f_size;
    int file_offset = 0;
    for(int i=0 ; i<page_num ; i++) {
        int read_bytes = total_read_bytes < PGSIZE ? total_read_bytes : PGSIZE;

        struct file_read_info *fri = malloc(sizeof(struct file_read_info));
        struct file *read_file = file_reopen(t->fdt[fd]);
        file_seek(read_file, file_offset);
        fri->file = read_file;
        fri->read_bytes = read_bytes;

        pagedir_set_page(t->pagedir, t->spt, next_page, true, fri);
        struct spte *spte = sup_page_get_page(t->spt, next_page);

        file_offset += read_bytes;
        total_read_bytes -= read_bytes;
        next_page += PGSIZE;
    }
  
    struct mmap *mmap = malloc(sizeof(struct mmap));
    mmap->file_size = f_size;
    mmap->page = addr;
    t->mmapt[fd] = mmap;
    return fd;
}

void 
mmap_unmap(int mapping) {
    struct thread *t = thread_current();
    struct mmap *mmap = t->mmapt[mapping];
    int page_num = mmap_get_page_num(mmap);  
    for(int i=0 ; i<page_num ; i++) {
        void *page = mmap->page + i * PGSIZE;
        struct spte *spte = sup_page_get_page(t->spt, page);
        if(pagedir_is_dirty(t->pagedir, page)) {
            file_write(spte->fri->file, page, spte->fri->read_bytes);
        }
        pagedir_clear_page(t->pagedir, page);
        sup_page_clear_page(t->spt, spte);
    }
    t->mmapt[mapping] = NULL;
    free(mmap);
}

int mmap_get_page_num(struct mmap *mmap) {
    return DIV_ROUND_UP(mmap->file_size, PGSIZE);
}