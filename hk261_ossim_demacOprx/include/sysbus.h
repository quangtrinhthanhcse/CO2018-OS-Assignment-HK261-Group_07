/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 *
 * demacOprx release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 *
 * system bus protoype header file
 */

#ifndef SYSTEM_BUS_H
#define SYSTEM_BUS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of memory banks supported
#define MAX_MEMORY_BANKS 8

// Memory bank handle type
typedef int memory_bank_t;

// Invalid memory bank handle
#define MEMORY_BANK_INVALID (-1)

// Initialize a memory bank with given size in bytes
memory_bank_t memory_bank_init(size_t size_bytes);

// Cleanup all memory banks
void memory_bank_cleanup(void);

// Get pointer to locate data at offset
void* memory_bank_get_ptr(uint64_t offset);

// Get bank size in bytes
size_t memory_bank_get_size(void);

// Check if memory bank is initialized
int memory_bank_is_available(void);

// Map memory (testbench bank or OS RAM) at bus address 0 for one transfer of len bytes;
// the 32-bit bus has no byte enables: bytes outside the transfer read as 0,
// writes outside [dst, dst + len) are dropped
void sysbus_map(void *mem, uint64_t size, uint64_t src, uint64_t dst, uint64_t len);

// DPI Callback: Called from RTL when DMA read access
int dpi_bank_read32_legacy(long long offset);

// DPI Callback: Called from RTL when DMA write access
int dpi_bank_write32_legacy(long long offset, int value);

// Completion reports on (1, default) or off (0): off while the chip copies a piece
// that is not the last one of its dma_memcpy
void sysbus_irq_enable(int on);

// DPI Callback: Called from RTL when DMA transfer completes
void dma_done_callback(void);
#ifdef __cplusplus
}
#endif

#endif // SYSTEM_BUS_H