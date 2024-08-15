#ifndef FILESYS_BUFFER_CACHE_H
#define FILESYS_BUFFER_CACHE_H

#include "devices/block.h"
#include <stdbool.h>
#include "filesys/off_t.h"

#define BUFFER_CACHE_ENTRY_SIZE  64

struct buffer_cache_entry {
  bool use;
  bool dirty;
  block_sector_t sector;
  uint8_t data[512];
};

void buffer_cache_init();
struct buffer_cache_entry *read_cache(block_sector_t sector);
void write_cache(struct buffer_cache_entry *cache, off_t cache_data_offset, void *buffer, size_t size);
void set_cache(block_sector_t sector, uint8_t *data);
void remove_cache(block_sector_t sector);
struct buffer_cache_entry *get_buffer_entry(block_sector_t sector);
struct buffer_cache_entry *caching(block_sector_t sector);
void write_back(struct buffer_cache_entry *cache);
void write_back_all();
void read_ahead(block_sector_t sector);

#endif