#include "filesys/buffer-cache.h"
#include "filesys/filesys.h"
#include "threads/thread.h"
#include "devices/timer.h"
#include "threads/malloc.h"
#include <string.h>

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
read_cache(block_sector_t sector) {
  lock_acquire(&buffer_cache_lock);
  struct buffer_cache_entry *cache = get_buffer_entry(sector);
  if(cache == NULL) {
    cache = caching(sector);
  }
  lock_release(&buffer_cache_lock);

  return cache;
}

/* cache data의 offset 위치부터 buffer의 size만큼 write */
void
write_cache(struct buffer_cache_entry *cache, off_t cache_data_offset, void *buffer, size_t size) {
  ASSERT(cache_data_offset + size <= BLOCK_SECTOR_SIZE);
  lock_acquire(&buffer_cache_lock);
  if(memcmp(cache->data + cache_data_offset, buffer, size) !=0) {
    memcpy(cache->data + cache_data_offset, buffer, size);
    cache->dirty = true;
  }
  lock_release(&buffer_cache_lock);
}

void
set_cache(block_sector_t sector, uint8_t *data) {
  lock_acquire(&buffer_cache_lock);
  int idx = sector % BUFFER_CACHE_ENTRY_SIZE;
  struct buffer_cache_entry *cache = &buffer_cache[idx];
  write_back(cache);
  cache->use = true;
  cache->dirty = true;
  cache->sector = sector;
  memcpy(cache->data, data, BLOCK_SECTOR_SIZE);
  lock_release(&buffer_cache_lock);
}

void
remove_cache(block_sector_t sector) {
  lock_acquire(&buffer_cache_lock);
  struct buffer_cache_entry *cache = get_buffer_entry(sector);
  if(cache != NULL) {
    cache->use = false;
  }
  lock_release(&buffer_cache_lock);
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
  write_back(cache);
  cache->use = true;
  cache->dirty = false;
  cache->sector = sector;
  block_read (fs_device, sector, buffer_cache[idx].data);
  lock_release(&buffer_cache_lock);

  return cache;
}

void
write_back(struct buffer_cache_entry *cache) {
  ASSERT(cache != NULL);
  lock_acquire(&buffer_cache_lock);
  if(cache->use && cache->dirty) {
    block_write(fs_device, cache->sector, cache->data);
    cache->use = false;
    cache->dirty = false;
  }
  lock_release(&buffer_cache_lock);
}

void
write_back_all() {
  for(int i=0 ; i<BUFFER_CACHE_ENTRY_SIZE ; i++) {
      write_back(&buffer_cache[i]);
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