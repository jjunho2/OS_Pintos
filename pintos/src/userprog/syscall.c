#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
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
      if (f->fd == fd)
        return f->_file;
  }
  return NULL;
}

static int
get_user (const uint8_t *uaddr)
{
  if(uaddr>=PHYS_BASE) return -1;
    int result;
      asm ("movl $1f, %0; movzbl %1, %0; 1:"
                 : "=&a" (result) : "m" (*uaddr));
        return result;
}
 
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  if(udst>=PHYS_BASE) return false;
    int error_code;
      asm ("movl $1f, %0; movb %b2, %1; 1:"
                 : "=&a" (error_code), "=m" (*udst) : "q" (byte));
        return error_code != -1;
}

static bool get_user_for(void *esp, int argc){
  int i;
  for(i=0;i<argc;i++){
    if(get_user(((uint8_t *)esp) + i)==-1)
      return false;
  }
  return true;
}

static bool get_user_while(void *esp){
  int i = 0, temp;
  while(1){
    temp = get_user(((char *)esp) + i);
    if(temp == '\0' || temp == -1) break;
    i++;
  }
  if (temp == '\0') return true;
  else return false;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
  list_init (&file_list);
}

static void
syscall_handler (struct intr_frame *f) 
{

  if(!get_user_for(f->esp, 4)){
    exit(-1);
    return;
  }

  void * esp = f->esp;
  int syscall_num = *(int*)esp;
  int arg1, arg2, arg3;

  switch(syscall_num){
    case SYS_HALT:
      break;
    case SYS_EXIT:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp+4);
      exit(arg1);
      break;
    case SYS_EXEC:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp + 4);
      if(!get_user_while(arg1)) exit(-1);
      f->eax = exec(arg1);
      break;
    case SYS_WAIT:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp + 4);
      f->eax = wait(arg1);
      break;
    case SYS_CREATE:
      if(!get_user_for(f->esp+4, 8)) exit(-1);
      arg1 = *(int*)(esp + 4); 
      arg2 = *(int*)(esp + 8); 
      if(!get_user_while(arg1)) exit(-1);
      f->eax = create(arg1,arg2);
      break;
    case SYS_REMOVE:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp + 4);
      if(!get_user_while(arg1)) exit(-1);
      f->eax = remove(arg1);
      break;
    case SYS_OPEN:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp + 4);
      if(!get_user_while(arg1)) exit(-1);
      f->eax = open(arg1);
      break;
    case SYS_FILESIZE:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp + 4);
      f->eax = filesize(arg1);
      break;
    case SYS_READ:
      if(!get_user_for(f->esp+4, 12)) exit(-1);
      arg1 = *(int*)(esp + 4);
      arg2 = *(int*)(esp + 8);
      arg3 = *(int*)(esp + 12);
      if(!get_user_for(arg2, 1) || !get_user_for(arg2+arg3, 1)) exit(-1);
      f->eax = read(arg1,arg2,arg3);
      break;
    case SYS_WRITE:
      if(!get_user_for(f->esp+4, 16)) exit(-1);
      arg1 = *(int*)(f->esp + 4);
      arg2 = *(int*)(f->esp + 8);
      arg3 = *(int*)(f->esp + 12);
      if(!get_user_for(arg2, 1) || !get_user_for(arg2+arg3, 1)) exit(-1);
      f->eax = write(arg1, arg2, arg3);
      break;
    case SYS_SEEK:
      if(!get_user_for(f->esp+4, 8)) exit(-1);
      arg1 = *(int*)(esp + 4);
      arg2 = *(int*)(esp + 8);
      seek(arg1,arg2);
      break;
    case SYS_TELL:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp + 4);
      f->eax = tell(arg1);
      break;
    case SYS_CLOSE:
      if(!get_user_for(f->esp+4, 4)) exit(-1);
      arg1 = *(int*)(esp + 4);
      close(arg1);
      break;
    case SYS_MMAP:
      break;
    case SYS_MUNMAP:
      break;
    case SYS_CHDIR:
      break;
    case SYS_MKDIR:
      break;
    case SYS_READDIR:
      break;
    case SYS_ISDIR:
      break;
    case SYS_INUMBER:
      break;
    default:
      exit(-1);
      break;
  }
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
  struct pid_node *p;
  struct list_elem *e;
  struct file_node *f;

  if(!(cur->is_kernel))
    printf("%s: exit(%d)\n",cur->full_name, status);
  
  p=pid2pid_node(cur->tid);
  p->returned_val=status;
  p->is_returned=true;

  for (e=list_begin(&cur->file_opened);e!=list_end(&cur->file_opened);e=list_next(e))
  {
      f=list_entry(e, struct file_node, thread_list_elem);
      list_remove(&f->all_list_elem);
      list_remove(&f->thread_list_elem);
      file_close(f->_file);
      //palloc_free_page(f);
  }
  
  //palloc_free_page();
  //cur의 fd전부close.
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
{
  int state;
  struct thread *cur=thread_current();
  struct list_elem *e;
  struct thread *t;
  for (e=list_begin(&(cur->child_tid_list));e!=list_end(&(cur->child_tid_list));e=list_next(e))
  {
    t = list_entry(e, struct thread, tid_list_node);
    if (t->tid == pid) break;
  }
  if(t->tid==pid)
    return (process_wait(pid));
  else
    return -1;
}

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

  if(f==NULL)
    return (-1);

  fn = palloc_get_page (2);
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
  {
    lock_release(&file_lock);
    return -1;
  }

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
    {
      lock_release(&file_lock);
      return -1;
    }
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
    {
      lock_release(&file_lock);
        return -1;
    }

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
  {
    lock_release(&file_lock);
      return;
  }

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
  {
    lock_release(&file_lock);
      return -1;
  }
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
      if(f->fd==fd)
        break;
  }
  if(f->fd!=fd)
  {
    lock_release(&file_lock);
    return;
  }

  list_remove(&f->all_list_elem);
  list_remove(&f->thread_list_elem);

  file_close(f->_file);
  palloc_free_page(f);
  lock_release(&file_lock);
  return ;
}

