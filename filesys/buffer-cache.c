#include "filesys/buffer-cache.h"
#include "filesys/filesys.h"
#include "threads/thread.h"
#include "devices/timer.h"

static struct buffer_cache_entry buffer_cache[BUFFER_CACHE_ENTRY_SIZE];
static struct lock buffer_cache_lock;

static void write_back_periodically();

void buffer_cache_init() {
    lock_init (&buffer_cache_lock);
    thread_create("buffer-cache-evictor", PRI_DEFAULT, write_back_periodically, NULL);
}

struct buffer_cache_entry *
get_buffer_entry(block_sector_t sector) {
  for(int i=0 ; i<BUFFER_CACHE_ENTRY_SIZE ; i++) {
    lock_acquire(&buffer_cache_lock);
    if(buffer_cache[i].use &&  buffer_cache[i].sector == sector) {
      lock_release(&buffer_cache_lock);
      return &buffer_cache[i];
    }
    lock_release(&buffer_cache_lock);
  }
  return NULL;
}

void
caching(block_sector_t sector) {
  lock_acquire(&buffer_cache_lock);
  struct buffer_cache_entry *cache = &buffer_cache[sector];
  if(cache->use && cache->dirty) {
    write_back(cache);
  }

  cache->use = true;
  cache->dirty = false;
  cache->sector = sector;
  block_read (fs_device, sector, &(buffer_cache[sector].data));
  lock_release(&buffer_cache_lock);
}

void
write_back(struct buffer_cache_entry *cache) {
  block_write(fs_device, cache->sector, &cache->data);
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

static void 
write_back_periodically() {
  while(true) {
    write_back_all();
    timer_msleep(3000);
  }
}