/**
  * @file  : sram_alloc.h
  * @brief : contains functions prototypes, error enumeration
  */
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
  ERR_ALLOC_SYS_NOTFOUND    = -7,
  ERR_ALLOC_ID_INVALID      = -8,
  ERR_ALLOC_RW_EXCEED       = -9,
  ERR_ALLOC_USED            = -10,
};

/**
 * @brief: Mount allocation system in SRAM from ALLOC_START_ADDR
 *         With a size of 64kbyte. This size is limited by the addressing inode.
 * @param
 *
 * @return: OK, debug will show more details.
 */
int mount_allocsystem(void);

/**
 * @brief: Allocate a space of SRAM by an index
 *
 * @param alloc_id index, each application should chose one index
 *        and use it for all operations.
 *
 * @param : bytenbr size in bytes. Note that allocation is done in blocks of 512byte
 *          chosing for example 128byte will still allocate 512byte
 *          chosing for example 682byte will alocate 1024 (512*2)

 * @return: 0 (ALLOC_OK) or negativ number with error code. see error enum
 */
int sram_block_malloc(int alloc_id, size_t bytenbr);

/**
 * @brief: free the allocation. to make a bigger one maybe.
 *
 * @param alloc_id the index of allocation
 *
 * @return: 0 (ALLOC_OK) or negativ number with error code. see error enum
 */
int sram_block_free(int alloc_id);

/**
 * @brief: write to the allocation space a buffer of length len
 *
 * @param alloc_id : index of allocation
 * @param pos : byte offset of the allocation where to start writing the data
 * @param buffer : pointer to a data structure, array or single data.
 * @param len : length in bytes of the data written
 *
 * @return: 0 (ALLOC_OK) or negativ number with error code. see error enum
 */
int sram_alloc_write(int alloc_id, uint32_t pos, void *buffer, uint32_t len);

/**
 * @brief: Read from the allocation space a buffer of length len
 *
 * @param alloc_id : index of allocation
 * @param pos : byte offset of the allocation where to start reading data
 * @param buffer : pointer to a data structure, array or single data.
 * @param len : length in bytes of the data read
 *
 * @return: 0 (ALLOC_OK) or negativ number with error code. see error enum
 */
int sram_alloc_read(int alloc_id, uint32_t pos, void *buffer, uint32_t len);

/**
 * @brief: Getters for system data or for an allocation id.
 *
 * @param void, or alloc_id index of allocation
 *
 * @return: nbr of data, or Error Code.
 */
int sram_getentrynbr(void);
int sram_gettotalblocks(void);
int sram_getblocksize(void);
int sram_getfreeblocks(void);
int sram_getusedblocks(void);
int sram_getalloc_size(int alloc_id);
int sram_getalloc_valid(int alloc_id);

#endif // SRAM_ALLOC_H
