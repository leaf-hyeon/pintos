#include "filesys/buffer-cache.h"
#include "filesys/filesys.h"
#include "threads/thread.h"
#include "devices/timer.h"
#include "threads/malloc.h"

static struct buffer_cache_entry buffer_cache[BUFFER_CACHE_ENTRY_SIZE];
static struct lock buffer_cache_lock;

struct sector_arg {
  block_sector_t sector;
  struct list_elem elem;
};
static struct list sector_arg_list;
static struct lock sector_arg_list_lock;
static struct semaphore sector_arg_queue;

static void write_back_periodically();
static void read_ahead_task();

void buffer_cache_init() {
    lock_init (&buffer_cache_lock);
    list_init (&sector_arg_list);
    lock_init (&sector_arg_list_lock);
    sema_init (&sector_arg_queue, 0);
    thread_create("buffer-cache-evictor", PRI_DEFAULT, write_back_periodically, NULL);
    thread_create("read-ahead", PRI_DEFAULT, read_ahead_task, NULL);
}

struct buffer_cache_entry *
get_buffer_entry(block_sector_t sector) {
  int idx = sector % BUFFER_CACHE_ENTRY_SIZE;
  lock_acquire(&buffer_cache_lock);
  if(buffer_cache[idx].use &&  buffer_cache[idx].sector == sector) {
    lock_release(&buffer_cache_lock);
    return &buffer_cache[idx];
  }
  lock_release(&buffer_cache_lock);

  return NULL;
}

struct buffer_cache_entry *
caching(block_sector_t sector) {
  int idx = sector % BUFFER_CACHE_ENTRY_SIZE;
  lock_acquire(&buffer_cache_lock);
  struct buffer_cache_entry *cache = &buffer_cache[idx];
  // 이미 저장된 
  if(cache->use && cache->dirty) {
    write_back(cache);
  }

  cache->use = true;
  cache->dirty = false;
  cache->sector = sector;
  block_read (fs_device, sector, buffer_cache[idx].data);
  lock_release(&buffer_cache_lock);

  return cache;
}

void
write_back(struct buffer_cache_entry *cache) {
  block_write(fs_device, cache->sector, cache->data);
}

void
write_back_all() {
  for(int i=0 ; i<BUFFER_CACHE_ENTRY_SIZE ; i++) {
      lock_acquire(&buffer_cache_lock);
      struct buffer_cache_entry *cache = &buffer_cache[i];
      if(cache->use && cache->dirty) {
        write_back(cache);
        cache->use = false;
        cache->dirty = false;
      }
      lock_release(&buffer_cache_lock);
  }
}

void
read_ahead(block_sector_t sector) {
  struct sector_arg *arg = malloc(sizeof(struct sector_arg));
  arg->sector = sector;
  lock_acquire(&sector_arg_list_lock);
  list_push_back(&sector_arg_list, &arg->elem);
  sema_up(&sector_arg_queue);
  lock_release(&sector_arg_list_lock);
}

static void
read_ahead_task() {
  while(true) {
    sema_down(&sector_arg_queue);
    lock_acquire(&sector_arg_list_lock);
    struct sector_arg *sector_arg = list_entry(list_pop_front(&sector_arg_list), struct sector_arg, elem);
    lock_release(&sector_arg_list_lock);

    lock_acquire(&buffer_cache_lock);
    struct buffer_cache_entry *cache = get_buffer_entry(sector_arg->sector);
    if(cache != NULL) {
      lock_release(&buffer_cache_lock);
      continue;
    }
    caching(sector_arg->sector);
    lock_release(&buffer_cache_lock);
    free(sector_arg);
  }
}

static void 
write_back_periodically() {
  while(true) {
    write_back_all();
    timer_msleep(3000);
  }
}