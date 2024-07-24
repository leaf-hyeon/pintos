#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "lib/kernel/stdio.h"
#include "threads/synch.h"

static struct lock filesys_lock;

static void syscall_handler (struct intr_frame *);
static bool validate_user_stack_args (void *addr);
static bool check_is_unmapped_user_vaddr (void *addr);
static bool validate_fd(int fd);

static void halt();
static void exit(int status);
static tid_t exec(void *task);
static int wait(tid_t tid);
static bool create(char *file_name, unsigned int initial_size);
static bool remove(char *file_name);
static int open(char *file_name);
static int filesize(int fd);
static int read(int fd, void *buffer, unsigned size);
static int write (int fd, void *buffer, unsigned size);
static void seek (int fd, unsigned position);
static unsigned tell (int fd);
static void close (int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f) 
 {

  if(!validate_user_stack_args(f->esp)) {
    exit(-1);
  }
  int system_call_number = *(int *)(f->esp);
  printf("syscall! number is:%d\n", system_call_number);
  switch(system_call_number) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      exit(*(int *)(f->esp + 4));
      break;
    case SYS_EXEC:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      f->eax = exec(*(char **)(f->esp + 4));
      break;
    case SYS_WAIT:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      f->eax = wait(*(tid_t *)(f->esp + 4));
      break;
    case SYS_CREATE:
      // 이건 왜 16 부터냐...
      if(!validate_user_stack_args(f->esp + 16) || !validate_user_stack_args(f->esp + 20)) {
        exit(-1);
      }
      f->eax = create(*(char **)(f->esp + 16), *(unsigned int*)(f->esp + 20));
      break;
    case SYS_REMOVE:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      f->eax = remove(*(char **)(f->esp + 4));
      break;
    case SYS_OPEN:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      f->eax = open(*(char **)(f->esp + 4));
      break;
    case SYS_FILESIZE:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      f->eax = filesize(*(int *)(f->esp +4));
      break;
    case SYS_READ:
      // why 20?
      if(!validate_user_stack_args(f->esp + 20) || !validate_user_stack_args(f->esp + 24) || !validate_user_stack_args(f->esp + 28)) {
        exit(-1);
      }
      f->eax = read((int)*(uint32_t *)(f->esp + 20), *(void **)(f->esp +24), *(unsigned *)(f->esp +28));
      break;
    case SYS_WRITE:
      // 도대체 왜 20만큼 떨어진 곳에 argumnet들이 있지...?
      if(!validate_user_stack_args(f->esp + 20) || !validate_user_stack_args(f->esp + 24) || !validate_user_stack_args(f->esp + 28)) {
        exit(-1);
      }
      f->eax = write(*(int *)(f->esp +20), *(void **)(f->esp +24), *(unsigned *)(f->esp +28));
      break;
    case SYS_SEEK:
      if(!validate_user_stack_args(f->esp + 16) || !validate_user_stack_args(f->esp + 20)) {
        exit(-1);
      }
      seek(*(int *)(f->esp +16), *(unsigned *)(f->esp +20));
      break;
    case SYS_TELL:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      f->eax = tell(*(int *)(f->esp +4));
      break;
    case SYS_CLOSE:
      if(!validate_user_stack_args(f->esp+4)) {
        exit(-1);
      }
      close(*(int *)(f->esp + 4));
      break;
  }
}

static void
halt() {
  shutdown_power_off();
}

static void
exit(int status) {
  struct thread *cur = thread_current();
  printf("%s: exit(%d)\n", cur->name, status);
  thread_exit(status);
}

static tid_t
exec(void *task) {
  if(!validate_user_stack_args(task)) {
    exit(-1);
  }
  return process_execute(task);
  
}

static int
wait(tid_t tid) {
  return process_wait(tid);
}

static bool
create(char *file_name, unsigned int initial_size) {
  lock_acquire(&filesys_lock);
  if(!validate_user_stack_args(file_name)) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  bool success = filesys_create(file_name, initial_size);
  lock_release(&filesys_lock);

  return success;
}

static bool
remove(char *file_name) {
  lock_acquire(&filesys_lock);
  if(!validate_user_stack_args(file_name)) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  bool success = filesys_remove(file_name);
  lock_release(&filesys_lock);

  return success;
}

static int
open(char *file_name) {
  lock_acquire(&filesys_lock);
  if(!validate_user_stack_args(file_name)) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  int fd =  process_file_open(file_name);
  lock_release(&filesys_lock);

  return fd;
}

static int
filesize(int fd) {
  if(!validate_fd(fd)) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  struct file **fdt = thread_current()->fdt;
  struct file *file = fdt[fd];
  if(file == NULL) {
    exit(-1);
  }
  
  return file_length(file);
}

// return 읽은 바이트 수, 못 읽으면 -1, end of file 0, 0번은 keyboard
static int
read(int fd, void *buffer, unsigned size) {
  lock_acquire(&filesys_lock);
  if(!validate_user_stack_args(buffer) || !validate_fd(fd)) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  int read_byte = process_file_read(fd, buffer, size);
  lock_release(&filesys_lock);

  return read_byte;
}

// return 쓴 바이트 수, 파일 확장이 구현되지 않았기 때문에 쓸수 있는 만큼 쓴다.
static int
write(int fd, void *buffer, unsigned size) {
  lock_acquire(&filesys_lock);
  if(!validate_user_stack_args(buffer) || !validate_fd(fd)) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  int write_byte = process_file_write(fd, buffer, size);
  lock_release(&filesys_lock);

  return write_byte;
}

static void 
seek (int fd, unsigned position) {
  if(!validate_fd(fd)) {
    exit(-1);
  }
  return process_file_seek(fd, position);
}

static unsigned 
tell (int fd) {
  if(!validate_fd(fd)) {
    exit(-1);
  }
  return process_file_tell(fd);
}

static void 
close (int fd) {
  lock_acquire(&filesys_lock);
  if(!validate_fd(fd)) {
    lock_release(&filesys_lock);
    exit(-1);
  }
  struct file **fdt = thread_current()->fdt;
  process_file_close(fd);
  lock_release(&filesys_lock);
}

static bool
validate_user_stack_args(void *addr) {
  if(addr == NULL) { return false; };
  if(!is_user_vaddr(addr)) { return false; };
  if(check_is_unmapped_user_vaddr(addr)) { return false; };

  return true;
}

static bool
check_is_unmapped_user_vaddr(void *addr) {
  // 가상 메모리 구현시 로직 변경 필요
  if(is_user_vaddr(addr) && pagedir_get_page(thread_current()->pagedir, addr) == NULL) {
    return true;
  }

  return false;
}

static bool
validate_fd(int fd) {
  // reserved fd
  if(process_is_reserved_fd(fd)) {
    return true;
  }

  if(fd<0 || fd>=FDT_SIZE || thread_current()->fdt[fd] == NULL) {
    return false;
  }

  return true;
}
