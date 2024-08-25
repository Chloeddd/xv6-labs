// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

//lab cow
struct spinlock ref_lock; //保护pm_ref引用计数数组的访问
int pm_ref[(PHYSTOP - KERNBASE)/PGSIZE]; //记录物理内存页的引用计数

//将物理地址pa映射为引用计数数组pm_ref中的索引
uint64
getRefIdx(uint64 pa){
  return (pa-KERNBASE)/PGSIZE;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem"); //初始化内存分配锁
  initlock(&ref_lock, "pm_ref"); //初始化引用计数锁
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");


  acquire(&ref_lock);
  pm_ref[getRefIdx((uint64)pa)] --;
  if(pm_ref[getRefIdx((uint64)pa)] <= 0){ //防止物理页存在多个映射时，被回收
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }

  release(&ref_lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
    pm_ref[getRefIdx((uint64)r)] = 1; //物理页kalloc时，是第一次引用
  return (void*)r;
}

void *kcopy(void *pa) {
  acquire(&ref_lock);
  if(pm_ref[getRefIdx((uint64)pa)] <= 1){ //引用计数不大于1，表示该页面并未被共享
    release(&ref_lock);
    return pa;
  }

  char* new = kalloc(); //分配一个新的物理页面
  if(new == 0){
    release(&ref_lock);
    panic("out of memory");
    return 0;
  }

  memmove((void*)new, pa, PGSIZE); //复制

  pm_ref[getRefIdx((uint64)pa)] --; //引用计数减一
  release(&ref_lock);
  return (void*)new;
}

void
refup(void* pa){
  acquire(&ref_lock);
  pm_ref[getRefIdx((uint64)pa)] ++;
  release(&ref_lock);
}

void
refdown(void* pa){
  acquire(&ref_lock);
  pm_ref[getRefIdx((uint64)pa)] --;
  release(&ref_lock);
}