#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (int status);
void process_activate (void);
unsigned int process_file_open (char *file_name);
int process_file_read(int fd, void *buffer, unsigned size);
int process_file_write(int fd, void *buffer, unsigned size);
void process_file_close (int fd);
bool process_is_reserved_fd(int fd);

#endif /* userprog/process.h */
