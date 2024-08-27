// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BSIZE 13

struct {
  struct buf buf[NBUF];
  struct buf bucket[BSIZE];
  struct spinlock lock[BSIZE];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  int 

  for(int i=0;i<BSIZE;i++){
    initlock(&bcache.lock[1], "bcache");
    //初始化双向链表
    bcache.bucket[i].prev = &bcache.bucket[i];
    bcache.bucket[i].next = &bcache.bucket[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int bucketNo = b%BSIZE;
    //根据哈希桶，将buf插入双向链表
    b->next = bcache.bucket[bucketNo].next;
    b->prev = &bcache.bucket[bucketNo];
    initsleeplock(&b->lock, "buffer");
    bcache.bucket[bucketNo].next->prev = b;
    bcache.bucket[bucketNo].next = b;
  }

  
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bucketNo = blockno%BSIZE;

  acquire(&bcache.lock[bucketNo]);

  // Is the block already cached?
  for(b = bcache.bucket[bucketNo].next; b != &bcache.bucket[bucketNo]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[bucketNo]);
      acquiresleep(&b->lock);
      return b;
 
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.bucket[bucketNo].prev; b != &bcache.bucket[bucketNo]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[bucketNo]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  
  int bucketNo = b->blockno % BSIZE;

  acquire(&bcache.lock[bucketNo]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.bucket[bucketNo].next;
    b->prev = &bcache.bucket[bucketNo];
    bcache.bucket[bucketNo].next->prev = b;
    bcache.bucket[bucketNo].next = b;
  }
  
  release(&bcache.lock[bucketNo]);
}

void
bpin(struct buf *b) {
  int bucketNo = b->blockno % BSIZE;
  acquire(&bcache.lock[bucketNo]);
  b->refcnt++;
  release(&bcache.lock[bucketNo]);
}

void
bunpin(struct buf *b) {
  int bucketNo = b->blockno % BSIZE;
  acquire(&bcache.lock[bucketNo]);
  b->refcnt--;
  release(&bcache.lock[bucketNo]);
}


