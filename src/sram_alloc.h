#ifndef SRAM_ALLOC_H
#define SRAM_ALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Errors. */
enum
{
  ALLOC_OK                  =  0,
  ERR_ALLOC_FAILED          = -1,
  ERR_ALLOC_TABLE_FULL      = -2,
  ERR_BLOCK_TABLE_FULL      = -3,
  ERR_ALLOC_INVALID_BYTENBR = -4,
  ERR_ALLOC_BLOCKS_FULL     = -5,
  ERR_INVALID_ALLOC_HANDLE  = -6,
  ERR_ALLOC_SYS             = -7,
};

/* handle to identify an allocation. */
typedef struct
{
  uint8_t id;    /* ID of allocation */
  uint32_t size; /* length of allocation in bytes */
}sram_alloc_t;

int mount_allocsystem(void);
int sram_block_malloc(sram_alloc_t *handle, size_t bytenbr);
int sram_block_free(sram_alloc_t *handle);

int sram_getentrynbr(void);
int sram_getfreeblocks(void);
int sram_getusedblocks(void);

int sram_onblock_op(void *buffer, uint8_t blockstr, uint8_t blocknbr, bool write);


#endif // SRAM_ALLOC_H
