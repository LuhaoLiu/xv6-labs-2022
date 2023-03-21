#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "fcntl.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_mmap(void) {
    uint64 addr;
    int len, prot, flags, fd, offset;
    argaddr(0, &addr);
    argint(1, &len);
    argint(2, &prot);
    argint(3, &flags);
    argint(4, &fd);
    argint(5, &offset);
    if (addr != 0 || offset != 0) {
        panic("mmap: restrictions not followed");
    }

    struct proc* p = myproc();

    // map an unreadable file with PROT_READ
    if ((prot & PROT_READ) && !p->ofile[fd]->readable) {
        return ~0L;
    }

    // map an unwritable file with PROT_WRITE and MAP_SHARED
    if ((prot & PROT_WRITE) && (flags & MAP_SHARED) && !p->ofile[fd]->writable) {
        return ~0L;
    }

    addr = PGROUNDDOWN(p->max_addr - len);
    p->max_addr = addr;

    struct mmap_vma* vma = 0;
    for (int i = 0; i < MAXVMA; i++) {
        if (p->mmap_vma[i].addr == 0) {
            vma = &p->mmap_vma[i];
        }
    }
    if (!vma) {
        panic("no enough mmap vma");
    }
    vma->addr = addr;
    vma->len = len;
    vma->offset = offset;
    vma->prot = prot;
    vma->flags = flags;
    vma->fd = fd;
    vma->file = p->ofile[fd];
    vma->file->ref++;

    return addr;
}

uint64 sys_munmap(void) {
    uint64 addr;
    int len;
    argaddr(0, &addr);
    argint(1, &len);

    struct proc* p = myproc();
    struct mmap_vma* vma = 0;
    for (int i = 0; i < MAXVMA; i++) {
        if (p->mmap_vma[i].addr <= addr && p->mmap_vma[i].addr + p->mmap_vma[i].len >= addr + len) {
            // found corresponding entry in mmap vma
            vma = &p->mmap_vma[i];
            break;
        }
    }
    if (!vma) {
        return ~0L;
    }
    if (vma->flags & MAP_SHARED) {
        vma_writeback(vma, addr, len);
    }
    for (uint64 a = PGROUNDDOWN(addr); a < PGROUNDDOWN(addr + len); a += PGSIZE) {
        pte_t *pte = walk(p->pagetable, a, 0);
        if (!(PTE2PA(*pte) == 0)) {
            uvmunmap(p->pagetable, a, 1, 1);
        }
    }
    if (addr == vma->addr && len < vma->len) {
        vma->addr += len;
        vma->len -= len;
        vma->offset += len;
    } else if (addr > vma->addr && addr + len == vma->addr + vma->len) {
        vma->len -= len;
    } else if (addr == vma->addr && len == vma->len) {
        vma->file->ref--;
        vma->addr = 0;
    } else {
        panic("munmap: not supported addr type");
    }
    return 0;
}