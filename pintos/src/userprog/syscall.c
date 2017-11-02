#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/off_t.h"

typedef tid_t pid_t;

struct list file_list;

static struct lock file_lock;

static void syscall_handler (struct intr_frame *);
struct file* fd2file (int);
void syscall_init (void);

void halt (void);
void exit (int status);
pid_t exec (const char* cmd_line);
int wait (pid_t pid);

bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);


struct file*
fd2file (int fd)
{
  struct file_node *f;
  struct list_elem *e;

  for (e=list_begin(&file_list);e!=list_end(&file_list);e=list_next(e))
  {
      f = list_entry(e, struct file_node, all_list_elem);
      if (f->fd==fd)
        return f->_file;
  }
  return NULL;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
  list_init (&file_list);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  void * esp;
  //printf ("system call!\n");
  esp=f->esp;

  thread_exit ();
}

void
halt (void)
{
  shutdown_power_off();
}

void
exit (int status)
{
  struct thread *cur=thread_current();
  struct list_elem *et, *el;
  if(!(cur->is_kernel))
    printf("%s: exit(%d)\n",cur->full_name, status);
  
  palloc_free_page();

  //cur의 fd전부 close +  스택, 페이지 디렉토리  모두 free.
  thread_exit();
}

pid_t
exec (const char* cmd_line)
{
  tid_t t;
  t=process_execute(cmd_line);
  if(t==TID_ERROR)
    return -1;
  return (pid_t)t;
}

int
wait (pid_t pid)
{}

bool
create (const char *file, unsigned initial_size)
{
  return (filesys_create(file,initial_size));
}

bool
remove (const char *file)
{
  return (filesys_remove (file));
}

int
open (const char *file)
{
  static int file_num=1; // fd 0 for STDIN_FILENO, fd 1 for STDOUT_FILENO
  struct file *f = filesys_open (file);
  struct file_node *fn;
  struct thread *cur=thread_current();

  fn = palloc_get_page (0);
  if (fn == NULL)
    return (-1);
  fn->_file=f;
  fn->fd = ++file_num;
  list_push_back(&file_list,&(fn->all_list_elem));
  list_push_back(&(cur->file_opened),&(fn->thread_list_elem));

  return file_num;
}

int
filesize (int fd)
{
  int size;
  struct file *f;

  lock_acquire(&file_lock);

  f=fd2file(fd);
  if(f==NULL)
    return -1;

  size=(int)file_length(f);

  lock_release(&file_lock);
  return size;
}

int
read (int fd, void *buffer, unsigned size)
{
  struct file *f;
  int temp, i;

  lock_acquire(&file_lock);
 
  if(fd==STDIN_FILENO)
  {
    for(i=0;i<size;i++){
      char key = input_getc();
      *((char*)buffer + i) = key;
    }
    temp = size;
  }   
  else{
    f=fd2file(fd);
    if(f==NULL)
        return -1;

    temp=(int)file_read(f,buffer,(off_t)size);
  }
  lock_release(&file_lock);
  return temp;
}

int
write (int fd, const void *buffer, unsigned size)
{
  struct file *f;
  int temp;
  
  lock_acquire(&file_lock);

  if(fd==STDOUT_FILENO)
  {
    putbuf(buffer, size);
    temp = size;
  }
  else
  {
    f=fd2file(fd);

    if(f==NULL)
        return -1;

    temp=(int)file_write(f,buffer,(off_t)size);
  }
  lock_release(&file_lock);
  return temp;
}

void
seek (int fd, unsigned position)
{
  struct file *f;
  lock_acquire(&file_lock);
  f=fd2file(fd);
  if(f==NULL)
      return;

  file_seek(f, position);
  
  lock_release(&file_lock);
}

unsigned
tell (int fd)
{
  struct file *f;
  lock_acquire(&file_lock);
  f=fd2file(fd);
  if(f==NULL)
      return -1;
  file_tell(f);
  lock_release(&file_lock);
  return -1;
}

void
close (int fd)
{
  struct file_node *f;
  struct list_elem *e;
  struct thread *cur = thread_current();

  lock_acquire(&file_lock);
  for (e=list_begin(&file_list);e!=list_end(&file_list);e=list_next(e))
  {
      f = list_entry(e, struct file_node, all_list_elem);
  }
  list_remove(&f->all_list_elem);
  list_remove(&f->thread_list_elem);

  file_close(f);
  lock_release(&file_lock);
  return ;
}

