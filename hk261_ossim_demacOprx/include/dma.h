/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 *
 * demacOprx release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 *
 * DMA Interface Header
 */

#ifndef __DMA_H__
#define __DMA_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct pcb_t;

/*
 * do_dma - perform DMA function
 * caller: process control block
 * source: source memory address
 * destination: destination memory address
 * length: number of bytes to copy
 */
int do_dma(struct pcb_t *caller, uint64_t source, uint64_t destination, uint64_t length);

/*
 * Asynchronous DMA:
 * dma_submit - queue one transfer (do_dma arguments) for the co-processor and return at once;
 *              last: the final piece of one dma_memcpy, the only one that reports completion
 * dma_serve - run the queued transfers in order; the co-processor thread calls it
 * dma_stop - take no more requests: dma_serve returns once the queue is empty
 */
int dma_submit(struct pcb_t *caller, uint64_t source, uint64_t destination, uint64_t length, int last);
void dma_serve(void);
void dma_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __DMA_H__ */