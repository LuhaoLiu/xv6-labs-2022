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

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head;
} bcache;

#define HT_SIZE 37

struct {
    struct spinlock lock[HT_SIZE];
    struct buf* bp[HT_SIZE];
} ht_bcache;

void
binit(void)
{
//  struct buf *b;

  initlock(&bcache.lock, "bcache");

//  // Create linked list of buffers
//  bcache.head.prev = &bcache.head;
//  bcache.head.next = &bcache.head;
//  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
//    b->next = bcache.head.next;
//    b->prev = &bcache.head;
//    initsleeplock(&b->lock, "buffer");
//    bcache.head.next->prev = b;
//    bcache.head.next = b;
//  }

  for (int i = 0; i < HT_SIZE; i++) {
      initlock(&ht_bcache.lock[i], "bcache");
      if (i < NBUF) {
          initsleeplock(&bcache.buf[i].lock, "buffer");
          ht_bcache.bp[i] = &bcache.buf[i];
          bcache.buf[i].next = (struct buf*)0;
      }
      else ht_bcache.bp[i] = (struct buf*)0;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int id = blockno % HT_SIZE;
  acquire(&ht_bcache.lock[id]);

  // Is the block already cached?
  for(b = ht_bcache.bp[id]; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&ht_bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle any buffer whose refcnt is 0.
  // Search own hash bucket first.
  for(b = ht_bcache.bp[id]; b != 0; b = b->next){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&ht_bcache.lock[id]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // Search the whole bcache buffer.
  release(&ht_bcache.lock[id]); // avoid deadlock
  for (int i = (id + 1) % HT_SIZE; i != id; i = (i + 1) % HT_SIZE) {
      struct buf* prev = (struct buf*)0;
      acquire(&ht_bcache.lock[i]);
      for (b = ht_bcache.bp[i]; b != 0; b = b->next) {
          if (b->refcnt == 0) {
              // relink buffer
              if (prev == 0) {
                  // b is the first item in current bucket
                  ht_bcache.bp[i] = b->next;
              } else {
                  prev->next = b->next;
              }
              release(&ht_bcache.lock[i]);
              acquire(&ht_bcache.lock[id]);
              b->next = (struct buf*)0;
              b->dev = dev;
              b->blockno = blockno;
              b->valid = 0;
              b->refcnt = 1;
              if (ht_bcache.bp[id] == 0) {
                  ht_bcache.bp[id] = b;
              } else {
                  struct buf* tmp = (struct buf*)0;
                  for (tmp = ht_bcache.bp[id]; tmp->next != 0; tmp = tmp->next);
                  tmp->next = b;
              }
              release(&ht_bcache.lock[id]);
              acquiresleep(&b->lock);
              return b;
          }
          prev = b;
      }
      release(&ht_bcache.lock[i]);
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

//  acquire(&bcache.lock);
  b->refcnt--;
//  if (b->refcnt == 0) {
//    // no one is waiting for it.
//    b->next->prev = b->prev;
//    b->prev->next = b->next;
//    b->next = bcache.head.next;
//    b->prev = &bcache.head;
//    bcache.head.next->prev = b;
//    bcache.head.next = b;
//  }
  
//  release(&bcache.lock);
}

void
bpin(struct buf *b) {
//  acquire(&bcache.lock);
  b->refcnt++;
//  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
//  acquire(&bcache.lock);
  b->refcnt--;
//  release(&bcache.lock);
}


