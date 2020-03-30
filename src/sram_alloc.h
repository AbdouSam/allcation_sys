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
  ALLOC_ALREADY_ALLOCATED   =  1,
  ALLOC_OK                  =  0,
  ERR_ALLOC_FAILED          = -1,
  ERR_ALLOC_TABLE_FULL      = -2,
  ERR_BLOCK_TABLE_FULL      = -3,
  ERR_ALLOC_INVALID_BYTENBR = -4,
  ERR_ALLOC_BLOCKS_FULL     = -5,
  ERR_INVALID_ALLOC_HANDLE  = -6,
  ERR_ALLOC_SYS_NOTFOUND    = -7,
  ERR_ALLOC_ID_INVALID      = -8,
  ERR_ALLOC_WRITE_OVER      = -9,
};

/* handle to identify an allocation. */
typedef struct
{
  uint8_t id;    /* ID of allocation */
  uint32_t size; /* length of allocation in bytes */
}sram_alloc_t;

int mount_allocsystem(void);
int sram_block_malloc(int alloc_id, size_t bytenbr);
int sram_block_free(int alloc_id);

int sram_getentrynbr(void);
int sram_gettotalblocks(void);
int sram_getblocksize(void);
int sram_getfreeblocks(void);
int sram_getusedblocks(void);
int sram_getalloc_size(int alloc_id);
int sram_getalloc_valid(int alloc_id);


int sram_onblock_op(void *buffer, uint8_t blockstr, uint8_t blocknbr, bool write);

int sram_alloc_write(int alloc_id, uint32_t pos, void *buffer, uint32_t len);


#endif // SRAM_ALLOC_H
