#ifndef FILESYS_BUFFER_CACHE_H
#define FILESYS_BUFFER_CACHE_H

#include "devices/block.h"
#include <stdbool.h>

#define BUFFER_CACHE_ENTRY_SIZE  64

struct buffer_cache_entry {
  bool use;
  bool dirty;
  block_sector_t sector;
  uint8_t data[512];
};

void buffer_cache_init();
struct buffer_cache_entry * get_buffer_entry(block_sector_t sector);
void caching(block_sector_t sector);
void write_back(struct buffer_cache_entry *cache);
void write_back_all();

#endif