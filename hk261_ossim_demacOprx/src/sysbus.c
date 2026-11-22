/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 *
 * demacOprx release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 *
 * system bus - DMA chip access to memory (testbench bank or OS RAM)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "sysbus.h"

// Maximum number of memory banks supported
#define MAX_MEMORY_BANKS 8

// Memory bank structure
typedef struct {
    uint8_t* data;
    size_t size;
    int initialized;
} memory_bank_t_struct;

// Array of memory banks
static memory_bank_t_struct g_banks[MAX_MEMORY_BANKS] = {0};
static int g_bank_count = 0;

// Memory the DMA chip addresses, set by sysbus_map() before each transfer
static uint8_t* g_bus_mem = NULL;
static uint64_t g_bus_size = 0;
static uint64_t g_bus_rd_start = 0;
static uint64_t g_bus_rd_end = 0;
static uint64_t g_bus_wr_start = 0;
static uint64_t g_bus_wr_end = 0;

// Initialize a memory bank with given size in bytes
memory_bank_t memory_bank_init(size_t size_bytes) {
    // Find first free slot
    int free_slot = -1;
    for (int i = 0; i < MAX_MEMORY_BANKS; i++) {
        if (!g_banks[i].initialized) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) {
        printf("Maximum number of memory banks (%d) reached\n", MAX_MEMORY_BANKS);
        return MEMORY_BANK_INVALID;
    }

    if (size_bytes == 0) {
        printf("Invalid bank size: 0\n");
        return MEMORY_BANK_INVALID;
    }

    uint8_t* data = (uint8_t*)malloc(size_bytes);
    if (!data) {
        printf("Failed to allocate memory bank of %zu bytes\n", size_bytes);
        return MEMORY_BANK_INVALID;
    }

    memset(data, 0, size_bytes);
    g_banks[free_slot].data = data;
    g_banks[free_slot].size = size_bytes;
    g_banks[free_slot].initialized = 1;
    g_bank_count++;

    printf("Memory bank [%d] initialized: %zu bytes at address %p\n", free_slot, size_bytes, (void*)data);
    return free_slot;
}

// Cleanup memory bank
void memory_bank_cleanup(void) {
    for (int i = 0; i < MAX_MEMORY_BANKS; i++) {
        if (g_banks[i].initialized) {
            if (g_banks[i].data) {
                free(g_banks[i].data);
                g_banks[i].data = NULL;
            }
            g_banks[i].size = 0;
            g_banks[i].initialized = 0;
            g_bank_count--;
            printf("Memory bank [%d] cleaned up\n", i);
        }
    }
}

int memory_bank_is_available(void) {
    // Check if at least one memory bank is initialized
    for (int i = 0; i < MAX_MEMORY_BANKS; i++) {
        if (g_banks[i].initialized) {
            return 1;
        }
    }
    return 0;
}

// Get pointer to locate data at offset
void* memory_bank_get_ptr(uint64_t offset) {
    if (offset >= g_banks[0].size) {
        return NULL;
    }

    return g_banks[0].data + offset;
}

// Get bank size in bytes
size_t memory_bank_get_size() {
    return g_banks[0].size;
}

void sysbus_map(void *mem, uint64_t size, uint64_t src, uint64_t dst, uint64_t len) {
    g_bus_mem = (uint8_t*)mem;
    g_bus_size = size;
    g_bus_rd_start = src;
    g_bus_rd_end = src + len;
    g_bus_wr_start = dst;
    g_bus_wr_end = dst + len;
}


#ifdef __cplusplus
extern "C" {
#endif

/**
 * DPI Callback: Called from RTL when DMA read access
 */
int dpi_bank_read32_legacy(long long offset) {
    uint32_t value = 0;

    // Byte by byte (spec 2.3.2): same byte order on any host; bytes outside the transfer
    // (source, or destination for the chip's read-back) and past the memory end read as 0
    for (int i = 0; i < 4; i++) {
        uint64_t addr = (uint64_t)offset + i;
        if (g_bus_mem && addr < g_bus_size &&
            ((addr >= g_bus_rd_start && addr < g_bus_rd_end) ||
             (addr >= g_bus_wr_start && addr < g_bus_wr_end)))
            value |= (uint32_t)g_bus_mem[addr] << (8 * i);
    }
    return (int)value;
}

/**
 * DPI Callback: Called from RTL when DMA write access
 */
int dpi_bank_write32_legacy(long long offset, int value) {
    // The last word of a transfer may stick out of the destination: drop those bytes
    for (int i = 0; i < 4; i++) {
        uint64_t addr = (uint64_t)offset + i;
        if (g_bus_mem && addr < g_bus_size &&
            addr >= g_bus_wr_start && addr < g_bus_wr_end)
            g_bus_mem[addr] = (uint8_t)((uint32_t)value >> (8 * i));
    }
    return 0;
}


static int g_irq_enable = 1;

void sysbus_irq_enable(int on) {
    g_irq_enable = on;
}

/**
 * DPI Callback: Called from RTL when DMA transfer completes
 */
void dma_done_callback(void)
{
    if (!g_irq_enable)
        return;   // not the last piece of its dma_memcpy
    printf("DMA completed\n");
    fflush(stdout);   // keep ordering vs. printf_IMPORTANT output
}

#ifdef __cplusplus
}
#endif

