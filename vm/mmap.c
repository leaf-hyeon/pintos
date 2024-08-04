#include "vm/mmap.h"
#include "threads/vaddr.h"
#include <round.h>
#include "threads/thread.h"
#include "filesys/file.h"
#include "vm/sup-page-table.h"
#include "userprog/pagedir.h"

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

        file_offset += read_bytes;
        total_read_bytes -= read_bytes;
        next_page += PGSIZE;
    }
  
    struct mmap *mmap = malloc(sizeof(struct mmap));
    mmap->page = addr;
    mmap->file = file_reopen(t->fdt[fd]);
    t->mmapt[fd] = mmap;
    return fd;
}

void 
mmap_unmap(int mapping) {
    struct thread *t = thread_current();
    struct mmap *mmap = t->mmapt[mapping];
    int page_num = mmap_get_page_num(mmap);  
    int total_read_bytes = file_length(mmap->file);
    for(int i=0 ; i<page_num ; i++) {
        int read_bytes = total_read_bytes < PGSIZE ? total_read_bytes : PGSIZE;
        void *page = mmap->page + i * PGSIZE;
        if(pagedir_is_dirty(t->pagedir, page)) {
            file_write(mmap->file, page, read_bytes);
        }
        pagedir_delete_page(t->pagedir, t->spt, page);
        total_read_bytes-=read_bytes;
    }
    t->mmapt[mapping] = NULL;
    file_close(mmap->file);
    free(mmap);
}

int mmap_get_page_num(struct mmap *mmap) {
    return DIV_ROUND_UP(file_length(mmap->file), PGSIZE);
}