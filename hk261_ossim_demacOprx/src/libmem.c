/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 *
 * demacOprx release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 *
 * System Library
 * Memory Module Library libmem.c
 */

// #ifdef MM_PAGING

#include "string.h"
#include "mm64.h"
#include "syscall.h"
#include "libmem.h"
#include "dma.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t kmm_lock = PTHREAD_MUTEX_INITIALIZER;

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  /*Allocate at the toproof */
  if (caller == NULL || caller->mm == NULL || alloc_addr == NULL || size == 0 || rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return -1;

#ifdef MM64
  if (size > (addr_t)PAGING64_MAX_PGN * PAGING64_PAGESZ)
    return -1;
#else
  if (size > (addr_t)PAGING_MAX_PGN * PAGING_PAGESZ)
    return -1;
#endif

  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct rgnode;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (cur_vma == NULL || caller->mm->symrgtbl[rgid].rg_start < caller->mm->symrgtbl[rgid].rg_end)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
 
    *alloc_addr = rgnode.rg_start;

    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /*Attempt to increate limit to get space */
  addr_t old_sbrk;
  struct vm_rg_struct *left = malloc(sizeof(struct vm_rg_struct));
  if (left == NULL)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * SYSCALL 1 sys_memmap
   */
  struct sc_regs regs;
  regs.a1 = SYSMEM_INC_OP;
  regs.a2 = vmaid;
#ifdef MM64
  regs.a3 = size;
#else
  regs.a3 = PAGING_PAGE_ALIGNSZ(size);
#endif  
  if (_syscall(caller->krnl, caller->pid, 17, &regs) != 0) /* SYSCALL 17 sys_memmap */
  {
    free(left);
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }
  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/
  if (old_sbrk + size < cur_vma->sbrk)
  {
    left->rg_start = old_sbrk + size;
    left->rg_end = cur_vma->sbrk;
    left->rg_next = NULL;
    enlist_vm_freerg_list(caller->mm, left);
  }
  else
    free(left);

  pthread_mutex_unlock(&mmvm_lock);
  return 0;

}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  pthread_mutex_lock(&mmvm_lock);

  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  /* TODO: Manage the collect freed region to freerg_list */
  struct vm_rg_struct *rgnode = get_symrg_byid(caller->mm, rgid);

  if (rgnode == NULL || rgnode->rg_start >= rgnode->rg_end)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  struct vm_rg_struct *freerg_node = malloc(sizeof(struct vm_rg_struct));
  if (freerg_node == NULL)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }
  freerg_node->rg_start = rgnode->rg_start;
  freerg_node->rg_end = rgnode->rg_end;
  freerg_node->rg_next = NULL;

  rgnode->rg_start = rgnode->rg_end = 0;
  rgnode->rg_next = NULL;

  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, freerg_node);

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*liballoc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int liballoc(struct pcb_t *proc, addr_t size, uint32_t reg_index)
{
  addr_t  addr;
  int val = __alloc(proc, 0, reg_index, size, &addr);
  if (val == -1)
  {
    return -1;
  }
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  /* By default using vmaid = 0 */
  return val;
}

/*libfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  int val = __free(proc, 0, reg_index);
  if (val == -1)
  {
    return -1;
  }
printf("%s:%d\n",__func__,__LINE__);
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif
  return 0;//val;
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{

  uint64_t pte = pte_get_entry(caller->mm, pgn, caller);

  if (!PAGING_PAGE_PRESENT(pte) || (pte & PAGING_PTE_SWAPPED_MASK))
  { /* Page is not online, make it actively living */
    addr_t vicpgn = 0;
    addr_t vicpte;
    addr_t vicfpn, swpfpn = 0;
    struct sc_regs regs;

    if (!(pte & PAGING_PTE_SWAPPED_MASK))
      return -1;

    /* TODO Initialize the target frame storing our variable */
    addr_t tgtfpn = PAGING_PTE_SWP(pte); 
    if (PAGING_PTE_SWPTYP(pte) != caller->krnl->active_mswp_id)
      return -1;

    if (MEMPHY_get_freefp(caller->krnl->mram, &vicfpn) == -1)
    {
      /* TODO: Play with your paging theory here */
      /* Find victim page */
      struct pgn_t *pg = caller->mm->fifo_pgn;
      if (pg == NULL)
      {
        return -1;
      }
      while (pg->pg_next != NULL)
      {
        pg = pg->pg_next;
      }
      vicpgn = pg->pgn;
      vicpte = pte_get_entry(caller->mm, vicpgn, caller);
      if (!PAGING_PAGE_PRESENT(vicpte) || (vicpte & PAGING_PTE_SWAPPED_MASK))
        return -1;
      vicfpn = PAGING_FPN(vicpte);

      /* Get free frame in MEMSWP */
      if (MEMPHY_get_freefp(caller->krnl->active_mswp, &swpfpn) == -1)
      {
        return -1;
      }

      /* TODO: Implement swap frame from MEMRAM to MEMSWP and vice versa*/

      /* TODO copy victim frame to swap 
       * SWP(vicfpn <--> swpfpn)
       * SYSCALL 1 sys_memmap
       */
      regs.a1 = SYSMEM_SWP_OP;
      regs.a2 = vicfpn;
      regs.a3 = swpfpn;
      if (_syscall(caller->krnl, caller->pid, 17, &regs) != 0)
      {
        MEMPHY_put_freefp(caller->krnl->active_mswp, swpfpn);
        return -1;
      }

      /* Update page table */
      if (pte_set_swap(caller, vicpgn, caller->krnl->active_mswp_id, swpfpn) != 0)
      {
        MEMPHY_put_freefp(caller->krnl->active_mswp, swpfpn);
        return -1;
      }
      find_victim_page(caller->mm, &vicpgn);
    }

    regs.a1 = SYSMEM_SWP_IN_OP;
    regs.a2 = vicfpn;
    regs.a3 = tgtfpn;
    if (_syscall(caller->krnl, caller->pid, 17, &regs) != 0)
    {
      MEMPHY_put_freefp(caller->krnl->mram, vicfpn);
      return -1;
    }

    /* Update its online status of the target page */
    if (pte_set_fpn(caller->mm, pgn, vicfpn, caller) != 0)
    {
      MEMPHY_put_freefp(caller->krnl->mram, vicfpn);
      return -1;
    }

    MEMPHY_put_freefp(caller->krnl->active_mswp, tgtfpn);

    enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(pte_get_entry(caller->mm,pgn, caller));

  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  #ifdef MM64
    int pgn = addr / PAGING64_PAGESZ;
    int off = addr % PAGING64_PAGESZ;
  #else
    int pgn = PAGING_PGN(addr);
    int off = PAGING_OFFST(addr);
  #endif
    int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  #ifdef MM64
    addr_t phyaddr = (addr_t)fpn * PAGING64_PAGESZ + off;
  #else
    addr_t phyaddr = (addr_t)fpn * PAGING_PAGESZ + off;
  #endif

  /* TODO 
   *  MEMPHY_read(caller->krnl->mram, phyaddr, data);
   *  MEMPHY READ 
   *  SYSCALL 17 sys_memmap with SYSMEM_IO_READ
   */
  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_READ;
  regs.a2 = phyaddr;
  regs.a3 = 0;

  if (_syscall(caller->krnl, caller->pid, 17, &regs) != 0)
    return -1;

  *data = (BYTE)regs.a3;

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  #ifdef MM64
    int pgn = addr / PAGING64_PAGESZ;
    int off = addr % PAGING64_PAGESZ;
  #else
    int pgn = PAGING_PGN(addr);
    int off = PAGING_OFFST(addr);
  #endif
    int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  #ifdef MM64
    addr_t phyaddr = (addr_t)fpn * PAGING64_PAGESZ + off;
  #else
    addr_t phyaddr = (addr_t)fpn * PAGING_PAGESZ + off;
  #endif
  /* TODO 
   *  MEMPHY_write(caller->krnl->mram, phyaddr, value);
   *  MEMPHY WRITE with SYSMEM_IO_WRITE 
   * SYSCALL 17 sys_memmap
   */

  struct sc_regs regs;
  regs.a1 = SYSMEM_IO_WRITE;
  regs.a2 = phyaddr;
  regs.a3 = value;                          

  return _syscall(caller->krnl, caller->pid, 17, &regs);
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  pthread_mutex_lock(&mmvm_lock);

  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  /* TODO Invalid memory identify */
  if (currg == NULL || cur_vma == NULL || currg->rg_start >= currg->rg_end || offset >= currg->rg_end - currg->rg_start) {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  int ret = pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}

/*libread - PAGING-based read a region memory */
int libread(
    struct pcb_t *proc, // Process executing the instruction
    uint32_t source,    // Index of source register
    addr_t offset,    // Source address = [source] + [offset]
    uint32_t* destination)
{
  BYTE data;
printf("%s:%d\n",__func__,__LINE__);
  int val = __read(proc, 0, source, offset, &data);

  if (val != 0)
  return val;

  *destination = (unsigned char)data;
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL || currg->rg_start >= currg->rg_end || offset >= currg->rg_end - currg->rg_start) /* Invalid memory identify */
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  int ret = pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}

/*libwrite - PAGING-based write a region memory */
int libwrite(
    struct pcb_t *proc,   // Process executing the instruction
    BYTE data,            // Data to be wrttien into memory
    uint32_t destination, // Index of destination register
    addr_t offset)
{
  int val = __write(proc, 0, destination, offset, data);
  if (val == -1)
  {
    return -1;
  }
#ifdef IODUMP
  /* TODO dump IO content (if needed) */
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
#endif

  return val;
}

#ifdef MM64
#define KMEM_PAGESZ  ((addr_t)PAGING64_PAGESZ)
#define KMEM_MAX_PGN ((addr_t)PAGING64_MAX_PGN)
typedef addr_t kpte_t;
#else
#define KMEM_PAGESZ  ((addr_t)PAGING_PAGESZ)
#define KMEM_MAX_PGN ((addr_t)PAGING_MAX_PGN)
typedef uint32_t kpte_t;
#endif
#define KMEM_LIMIT         (KMEM_MAX_PGN * KMEM_PAGESZ)
#define KMEM_ALIGN_UP(x)   ((((x) + KMEM_PAGESZ - 1) / KMEM_PAGESZ) * KMEM_PAGESZ)
#define KMEM_MAX_RG        128 

struct kmem_rg {
  int used;
  uint32_t pid;
  int rgid;
  struct vm_rg_struct rg;
  addr_t fpn0;
  addr_t npg;
};

static struct kmem_rg ksymrgtbl[KMEM_MAX_RG];
static addr_t kmem_top = 0;

static kpte_t *kwalk(struct krnl_t *krnl, addr_t va)
{
  addr_t pgn = va / KMEM_PAGESZ;
  if (krnl == NULL || krnl->krnl_pgd == NULL || pgn >= KMEM_MAX_PGN)
    return NULL;
  return (kpte_t *)&krnl->krnl_pgd[pgn];
}

struct kmem_rg *kmem_find_rg(uint32_t pid, int rgid)
{
  for (int i = 0; i < KMEM_MAX_RG; i++)
    if (ksymrgtbl[i].used && ksymrgtbl[i].pid == pid && ksymrgtbl[i].rgid == rgid)
      return &ksymrgtbl[i];
  return NULL;
}

int kmem_va2pa(struct krnl_t *krnl, addr_t va, addr_t *pa)
{
  kpte_t *pte = kwalk(krnl, va);
  if (pte == NULL || !PAGING_PAGE_PRESENT(*pte))
    return -1;
  *pa = (addr_t)PAGING_PTE_FPN(*pte) * KMEM_PAGESZ + (va % KMEM_PAGESZ);
  return 0;
}

static int kmem_take_contig_frames(struct memphy_struct *mp, addr_t npg, addr_t *first)
{
  addr_t nfr = (addr_t)mp->maxsz / KMEM_PAGESZ;
  if (npg == 0 || npg > nfr)
    return -1;

  unsigned char *isfree = calloc(nfr, 1);
  if (isfree == NULL)
    return -1;

  for (struct framephy_struct *f = mp->free_fp_list; f != NULL; f = f->fp_next)
    if (f->fpn < nfr)
      isfree[f->fpn] = 1;

  addr_t run = 0, s = 0;
  int found = 0;
  for (addr_t i = 0; i < nfr; i++)
  {
    run = isfree[i] ? run + 1 : 0;
    if (run == npg)
    {
      s = i + 1 - npg;
      found = 1;
      break;
    }
  }
  free(isfree);
  if (!found)
    return -1;

  struct framephy_struct **pp = &mp->free_fp_list;
  while (*pp != NULL)
  {
    struct framephy_struct *f = *pp;
    if (f->fpn >= s && f->fpn < s + npg)
    {
      *pp = f->fp_next;
      free(f);
    }
    else
      pp = &f->fp_next;
  }
  *first = s;
  return 0;
}

/*libkmem_malloc- alloc region memory in kmem
 *@caller: caller
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: memory size
 */

int libkmem_malloc(struct pcb_t * caller, uint32_t size, uint32_t reg_index)
{
  /* 
   * TODO: provide OS level management
   *       and forward the request to helper
   */
//addr_t  addr;
//int val = ...

  /* 
   * TODO: provide OS kmem allocation validation
   */
  addr_t addr = 0;
  if (caller == NULL || caller->krnl == NULL)
    return -1;
  if (size == 0 || reg_index >= PAGING_MAX_SYMTBL_SZ)
    return -1;

  /* kernel memory is not tied to a user VMA; vmaid 0 is only a tag */
  if (__kmalloc(caller, 0, (int)reg_index, (addr_t)size, &addr) == (addr_t)-1)
    return -1;

#ifdef KMEM_DEBUG
  printf("kmalloc: pid=%u rgid=%u size=%u -> kernel addr %lu\n",
         caller->pid, reg_index, size, (unsigned long)addr);
#endif
  return 0;
}


/*kmalloc - alloc region memory in kmem
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: memory size
 *@alloc_addr: allocated address
 */
addr_t __kmalloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  /* TODO: provide OS kernel memory allocation
   *       update krnl_pgd for OS kernel level management */

  //struct krnl_t *krnl = caller->krnl;
  //krnl->symrgtbl...
  //krnl->krnl_pgd ...
  
  if (caller == NULL || caller->krnl == NULL || alloc_addr == NULL)
    return (addr_t)-1;
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ || size == 0)
    return (addr_t)-1;

  struct krnl_t *krnl = caller->krnl;
  if (krnl->mram == NULL || krnl->krnl_pgd == NULL)
    return (addr_t)-1;

  pthread_mutex_lock(&kmm_lock);
  pthread_mutex_lock(&mmvm_lock);

  addr_t ret = (addr_t)-1;
  struct kmem_rg *slot = NULL;

  if (kmem_top == 0)
    kmem_top = KMEM_PAGESZ;

  if (kmem_find_rg(caller->pid, rgid) != NULL)
    goto out;

  for (int i = 0; i < KMEM_MAX_RG; i++)
    if (!ksymrgtbl[i].used) { slot = &ksymrgtbl[i]; break; }
  if (slot == NULL)
    goto out;

  addr_t start = kmem_top;
  addr_t end = start + size;
  addr_t aligned_end = KMEM_ALIGN_UP(end);
  if (end < start || aligned_end < end || aligned_end > KMEM_LIMIT)
    goto out;

  addr_t npg = (aligned_end - start) / KMEM_PAGESZ;
  addr_t fpn0;
  if (kmem_take_contig_frames(krnl->mram, npg, &fpn0) != 0)
    goto out;

  for (addr_t k = 0; k < npg; k++)
  {
    kpte_t *pte = kwalk(krnl, start + k * KMEM_PAGESZ);
    *pte = 0;
    SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
    CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
    SETVAL(*pte, fpn0 + k, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
  }

  kmem_top = aligned_end;
  slot->used = 1;
  slot->pid = caller->pid;
  slot->rgid = rgid;
  slot->rg.vmaid = vmaid;
  slot->rg.rg_start = start;
  slot->rg.rg_end = end;
  slot->rg.rg_next = NULL;
  slot->fpn0 = fpn0;
  slot->npg = npg;

  *alloc_addr = start;
  ret = 0;

out:
  pthread_mutex_unlock(&mmvm_lock);
  pthread_mutex_unlock(&kmm_lock);
  return ret;
}



/*libmem_dma_memcpy - DMA memcpy
 *@caller: caller
 *@src: source region
 *@dst: destination region
 *@offset: memory offset
 *@size: copied data size
 */
int libmem_dma_memcpy(struct pcb_t *caller, uint32_t src, uint32_t dst, uint32_t offset, uint32_t size)
{
  /*
   * The DMA chip only sees bus (physical) addresses and bypasses the MMU:
   * translate both regions page by page here, queue the pieces and return.
   */
#ifdef MM64
  const addr_t pagesz = PAGING64_PAGESZ;   /* RAM frames and page tables are 4 KB in 64-bit mode */
#else
  const addr_t pagesz = PAGING_PAGESZ;
#endif
  uint32_t rgid[2] = { src, dst };
  struct vm_rg_struct *rg[2] = { NULL, NULL };
  int kernel[2] = { 0, 0 };
  addr_t va, pa[2], done, len, rgsz;
  int i, pass, fpn, bad, ret = -1;

  /* Same order as __kmalloc: kernel regions and krnl_pgd, then the process memory */
  pthread_mutex_lock(&kmm_lock);
  pthread_mutex_lock(&mmvm_lock);

  for (i = 0; i < 2; i++)
  {
    /* A region id names a user region (alloc) or, failing that, a kmalloc region of the caller */
    if (rgid[i] < PAGING_MAX_SYMTBL_SZ)
    {
      rg[i] = &caller->mm->symrgtbl[rgid[i]];
      if (rg[i]->rg_end <= rg[i]->rg_start)
      {
        struct kmem_rg *krg = kmem_find_rg(caller->pid, (int)rgid[i]);

        rg[i] = krg ? &krg->rg : NULL;
        kernel[i] = 1;
      }
    }
    if (rg[i] == NULL || rg[i]->rg_end <= rg[i]->rg_start)
    {
      printf("dma_memcpy: region %u is not allocated\n", rgid[i]);
      goto out;
    }
    rgsz = rg[i]->rg_end - rg[i]->rg_start;
    if (offset > rgsz || size > rgsz - offset)
    {
      printf("dma_memcpy: offset %u + size %u is past the end of region %u\n",
             offset, size, rgid[i]);
      goto out;
    }
  }

  /* Pass 0 checks every page, pass 1 queues: a page that cannot be used queues nothing */
  for (pass = 0; pass < 2; pass++)
    for (done = 0; done < size; done += len)
    {
      len = size - done;
      for (i = 0; i < 2; i++)
      {
        va = rg[i]->rg_start + offset + done;
        if (kernel[i])
          bad = kmem_va2pa(caller->krnl, va, &pa[i]) != 0;   /* kernel page table (krnl_pgd) */
        else
        {
          /* The chip cannot take a page fault: bring the page into RAM before translating */
          bad = pg_getpage(caller->mm, va / pagesz, &fpn, caller) != 0 ||
                !PAGING_PAGE_PRESENT(pte_get_entry(caller->mm, va / pagesz, caller));
          if (!bad)
            pa[i] = (addr_t)fpn * pagesz + va % pagesz;
        }
        if (bad)
        {
          printf("dma_memcpy: page not in RAM: region %u, offset %lu\n",
                 rgid[i], (unsigned long)(offset + done));
          goto out;
        }
        /* One transfer stays inside one page of each region */
        if (len > pagesz - va % pagesz)
          len = pagesz - va % pagesz;
      }
      /*
       * shortcut: the process keeps running and its frames are not pinned, so nothing may
       * read or write these bytes (user or kmalloc), or swap out or free their pages, before
       * "DMA completed"; the chip copies outside kmm_lock/mmvm_lock. Park the process in
       * cpu_routine and skip DMA pages in find_victim_page when that matters.
       */
      if (pass == 1 && dma_submit(caller, pa[0], pa[1], len, done + len == size) != 0)
        goto out;
    }
  ret = 0;

out:
  pthread_mutex_unlock(&mmvm_lock);
  pthread_mutex_unlock(&kmm_lock);
  return ret;
}

/* kmem_rg_len - size in bytes of kernel region (pid,rgid), -1 if none */
static int kmem_rg_len(struct pcb_t *caller, int rgid, uint64_t *len)
{
  int ret = -1;
  if (caller == NULL || rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return -1;
  pthread_mutex_lock(&kmm_lock);
  struct kmem_rg *r = kmem_find_rg(caller->pid, rgid);
  if (r != NULL)
  {
    *len = (uint64_t)(r->rg.rg_end - r->rg.rg_start);
    ret = 0;
  }
  pthread_mutex_unlock(&kmm_lock);
  return ret;
}

/* umem_rg_len - size in bytes of user region rgid of the caller, -1 if none */
static int umem_rg_len(struct pcb_t *caller, int rgid, uint64_t *len)
{
  int ret = -1;
  if (caller == NULL || caller->mm == NULL || rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return -1;
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct *rg = &caller->mm->symrgtbl[rgid];
  if (rg->rg_end > rg->rg_start)
  {
    *len = (uint64_t)(rg->rg_end - rg->rg_start);
    ret = 0;
  }
  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}

/* in_range - [offset, offset+size) lies inside a region of length len (overflow safe) */
static int in_range(uint64_t len, uint32_t offset, uint32_t size)
{
  return (uint64_t)size <= len && (uint64_t)offset <= len - (uint64_t)size;
}

int libkmem_copy_from_user(struct pcb_t *caller, uint32_t source, uint32_t destination, uint32_t offset, uint32_t size)
{
  /* TODO: provide OS level management kmem
   */
  /*
   * TODO: Map kernel address range
   */
  //__read_user_mem(...)
  //__write_kernel_mem(...);

  uint64_t ulen, klen;

  if (caller == NULL || caller->krnl == NULL)
    return -1;
  if (source >= PAGING_MAX_SYMTBL_SZ || destination >= PAGING_MAX_SYMTBL_SZ)
    return -1;
  if (size == 0)
    return 0;

  if (umem_rg_len(caller, (int)source, &ulen) != 0 || !in_range(ulen, offset, size))
    return -1;
  if (kmem_rg_len(caller, (int)destination, &klen) != 0 || !in_range(klen, offset, size))
    return -1;

  for (uint32_t i = 0; i < size; i++)
  {
    BYTE data;
    if (__read_user_mem(caller, 0, (int)source, (addr_t)offset + i, &data) != 0)
      return -1;
    if (__write_kernel_mem(caller, 0, (int)destination, (addr_t)offset + i, data) != 0)
      return -1;
  }

  return 0;
}

int libkmem_copy_to_user(struct pcb_t *caller, uint32_t source, uint32_t destination, uint32_t offset, uint32_t size)
{
  /* TODO: provide OS level management kmem
   */
  /*
   * TODO: Map kernel address range
   */
  //__read_kernel_mem(...)
  //__write_user_mem(...);

  uint64_t klen, ulen;

  if (caller == NULL || caller->krnl == NULL)
    return -1;
  if (source >= PAGING_MAX_SYMTBL_SZ || destination >= PAGING_MAX_SYMTBL_SZ)
    return -1;
  if (size == 0)
    return 1;               

  if (kmem_rg_len(caller, (int)source, &klen) != 0 || !in_range(klen, offset, size))
    return -1;
  if (umem_rg_len(caller, (int)destination, &ulen) != 0 || !in_range(ulen, offset, size))
    return -1;

  for (uint32_t i = 0; i < size; i++)
  {
    BYTE data;
    if (__read_kernel_mem(caller, 0, (int)source, (addr_t)offset + i, &data) != 0)
      return -1;
    if (__write_user_mem(caller, 0, (int)destination, (addr_t)offset + i, data) != 0)
      return -1;
  }

  return 1;
}

/* kmem_access - one byte of a kernel region, read (is_write=0) or write. */
static int kmem_access(struct pcb_t *caller, int rgid, addr_t offset,
                       int is_write, BYTE *data, BYTE value)
{
  if (caller == NULL || caller->krnl == NULL)
    return -1;
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return -1;
  if (!is_write && data == NULL)
    return -1;

  struct krnl_t *krnl = caller->krnl;
  if (krnl->mram == NULL || krnl->krnl_pgd == NULL)
    return -1;

  int ret = -1;
  addr_t pa;

  pthread_mutex_lock(&kmm_lock);

  struct kmem_rg *r = kmem_find_rg(caller->pid, rgid);
  if (r != NULL &&
      offset < r->rg.rg_end - r->rg.rg_start &&
      kmem_va2pa(krnl, r->rg.rg_start + offset, &pa) == 0 &&
      pa < (addr_t)krnl->mram->maxsz) 
  {
    if (is_write)
      ret = MEMPHY_write(krnl->mram, pa, value);
    else
      ret = MEMPHY_read(krnl->mram, pa, data);
  }

  pthread_mutex_unlock(&kmm_lock);
  return ret;
}

/* umem_region - validated user region of caller (NULL if invalid).
 * Caller must hold mmvm_lock. */
static struct vm_rg_struct *umem_region(struct pcb_t *caller, int vmaid,
                                        int rgid, addr_t offset)
{
  if (caller == NULL || caller->mm == NULL || caller->krnl == NULL)
    return NULL;
  if (rgid < 0 || rgid >= PAGING_MAX_SYMTBL_SZ)
    return NULL;
  if (get_vma_by_num(caller->mm, vmaid) == NULL)
    return NULL;

  struct vm_rg_struct *rg = &caller->mm->symrgtbl[rgid];
  if (rg->rg_end <= rg->rg_start)
    return NULL;
  if (offset >= rg->rg_end - rg->rg_start)
    return NULL;
  return rg;
}

/*__read_kernel_mem - read value in kernel region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __read_kernel_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  /* TODO: provide OS memory operator for kernel memory region */
  //krnl->krnl_pgd ... or krnl->pgd ... based on kmem implementation strategy

  return kmem_access(caller, rgid, offset, 0, data, 0);
}

/*__write_kernel_mem - write a kernel region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __write_kernel_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  /* TODO: provide OS memory operator for kernel memory region */
  //krnl->krnl_pgd ... or krnl->pgd ... based on kmem implementation strategy

  return kmem_access(caller, rgid, offset, 1, NULL, value);
}

/*__read_user_mem - read value in user region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __read_user_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE *data)
{
  /* TODO: provide OS level management user memory access */
  //krnl->pgd ... or krnl->krnl_pgd  ... based on kmem implementation strategy

   if (data == NULL)
    return -1;

  pthread_mutex_lock(&mmvm_lock);   /* pg_getpage may swap pages */

  int ret = -1;
  struct vm_rg_struct *rg = umem_region(caller, vmaid, rgid, offset);
  if (rg != NULL)
    ret = pg_getval(caller->mm, (int)(rg->rg_start + offset), data, caller);

  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}


/*__write_user_mem - write a user region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@offset: offset to acess in memory region
 *@value: data value
 */
int __write_user_mem(struct pcb_t *caller, int vmaid, int rgid, addr_t offset, BYTE value)
{
  /* TODO: provide OS level management user memory access */
  //krnl->pgd ... or krnl->krnl_pgd  ... based on kmem implementation strategy

  pthread_mutex_lock(&mmvm_lock);

  int ret = -1;
  struct vm_rg_struct *rg = umem_region(caller, vmaid, rgid, offset);
  if (rg != NULL)
    ret = pg_setval(caller->mm, (int)(rg->rg_start + offset), value, caller);

  pthread_mutex_unlock(&mmvm_lock);
  return ret;
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  pthread_mutex_lock(&mmvm_lock);
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte = caller->mm->pgd[pagenum];

    if (PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->krnl->mram, fpn);
    }
    else
    {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->krnl->active_mswp, fpn);
    }
  }

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}


/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, addr_t *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;

  /* TODO: Implement the theorical mechanism to find the victim page */
  if (!pg)
  {
    return -1;
  }
  struct pgn_t *prev = NULL;
  while (pg->pg_next)
  {
    prev = pg;
    pg = pg->pg_next;
  }
  *retpgn = pg->pgn;
  if (prev == NULL)
    mm->fifo_pgn = NULL;      
  else
    prev->pg_next = NULL;

  free(pg);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (cur_vma == NULL || size <= 0 || newrg == NULL)
  return -1;

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (rgit->rg_start <= rgit->rg_end && (addr_t)size <= rgit->rg_end - rgit->rg_start)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        {                                /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
      break;
    }
    else
    {
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;

  return 0;
}

// #endif
