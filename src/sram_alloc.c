/**
  * @file  sram_alloc.c
  * @brief Implimentation of an allocation system for External SRAM
  * @note  this module is not intended for professional use, it is intended as an 
  *          educational tool to understand how file systems works.
  */

/**
 * @todo
 * - Limitation is the blocks table is limited to 128 block addressing only.
 *   We can make that more flexiblewe can increase the block size more thn 512
 *   but this will create a problem, one entry in block table should be 4 byte
 *   the index is 1 byte (256 max address)
 * - Many routines needs a LOCK  or a mutex, no mecanism is applyed for now.
 * - use queues for Writing Reading to SRAM instead of directly accessing the address
 */

#include "sram_alloc.h"
#include "sram_drv.h"
#include "debug.h"
#include "config.h"

#define BLOCK_DATA_STRUCT_LEN  4 /* Length of block in byte */
#define ALLOC_DATA_STRUCT_LEN  4 /* Length of alloc row in byte */
#define BLOCK_NUMBER          (SRAM_SIZE / BLOCK_SIZE) /* TOTAL BLOCKS nbr in sytsem */
#define BLOCK_TABLE_ROW_NBR   (BLOCK_SIZE / BLOCK_DATA_STRUCT_LEN )
#define ALLOC_TABLE_ROW_NBR   (BLOCK_SIZE / ALLOC_DATA_STRUCT_LEN)

/* Size in byte / 4 to get 32bit addresses
   with 512/ 4 = 128 Address to hold data about 128 allocation spaces. */

#define SUPER_BLKNBR          1    
#define BLOCK_TABLE_BLKNBR    1     
#define ALLOC_TABLE_BLKNBR    1     
#define DATA_BLKNBR           (BLOCK_NUMBER - SUPER_BLKNBR - \
                               BLOCK_TABLE_BLKNBR - ALLOC_TABLE_BLKNBR)

#define SUPER_STR_INDX        0
#define BLOCK_TABLE_STR_INDX  (SUPER_STR_INDX + SUPER_BLKNBR)
#define ALLOC_TABLE_STR_INDX  (BLOCK_TABLE_STR_INDX +  BLOCK_TABLE_BLKNBR)
#define DATA_STR_INDX         (ALLOC_TABLE_STR_INDX + ALLOC_TABLE_BLKNBR)

#define ALLOC_MAGIC_NBR          0xDE0FA751
#define ALLOC_SUPER_BLCOK_ADDR  (ALLOC_START_ADDR + 0 * BLOCK_SIZE) /* Block.0 */
#define ALLOC_BLOCK_TABLE_ADDR  (ALLOC_START_ADDR + 1 * BLOCK_SIZE) /* Block.1 */
#define ALLOC_FILE_TABLE_ADDR   (ALLOC_START_ADDR + 2 * BLOCK_SIZE) /* Block.2 */



/**
 * @brief  a block entry, holds data about a block
 * @warning It is essential for this struct to not exceed 32bit
 */
typedef struct 
{
  uint8_t index; /**< 0-255 indexing */
  uint8_t busy;  /**< 1: busy, 0: free*/
  int16_t next;  /**< Next index reserved (-1 of non) Warning (-127, 128) range. */
}block_t;
 
 /**
  * @brief allocation entry, hold data about an allocation
  * @warning It is essential for this struct to not exceed 32bit
  */
typedef struct 
{
  uint8_t index; /**< Id of the allocation space */
  uint8_t start; /**< Starting index */
  uint8_t len;   /**< nbr of blocks reserved */
  uint8_t used;  /**< Is this entry used or not. */
}alloc_t;

/**
  * @brief Super block hold data about the allocation system.
  * @warning It is essential for this struct to not exceed BLOCK_SIZE (512byte)
  */
typedef struct
{
  uint32_t  magicnbr;     /**< Special nbre to check whether a system is mounted. */
  uint32_t  bsize;        /**< Size of one block in bytes */
  uint32_t  blocknbr;     /**< total block nbr in sys */
  uint32_t  usedblocknbr; /**< total used data blocks nbr in sys */
  uint32_t  freeblocknbr; /**< total free data blocks nbr in sys */
  uint32_t  spblocknbr;   /**< nbr blocks reserve for super block */
  uint32_t  btblocknbr;   /**< nbr block reserved for block table */
  uint32_t  alblocknbr;   /**< nbr block reserved for allocation table*/
  uint32_t  datablocknbr; /**< nbr block reserved for data */
  uint32_t  spstrindex;   /**< super block start index. */
  uint32_t  btstrindex;   /**< Block table start index. */
  uint32_t  alstrindex;   /**< alloc table start index. */
  uint32_t  datastrindex; /**< data start index. */
  uint32_t  blentrynbr;   /**< nbr of rows in Block table. */
  uint32_t  alentrynbr;   /**< nbr of rows in Alloc table. */
  uint32_t  allocnbr;     /**< nbr of allocations made */
}superblock_t;

/**
 * @brief global array index blocks
 */
static block_t gblock_table[BLOCK_TABLE_ROW_NBR];

/** 
 * @brief global array alloc blocks
 */
static alloc_t galloc_table[ALLOC_TABLE_ROW_NBR];

/**
 *@brief the super block for sram mounting
 */
static superblock_t gsuper_block; 
static uint8_t local_buffer[BLOCK_SIZE];

#ifdef TEST
static void sram_onblock_op(void *buffer, uint8_t blockstr, uint8_t blocknbr, bool write)
{
  int i;
  uint32_t *data;
  uint32_t addr;
  uint32_t bytenbr;

  bytenbr = blocknbr * BLOCK_SIZE;

  if (write)
  {
    addr = ALLOC_START_ADDR + blockstr * BLOCK_SIZE;

    data = (uint32_t *)buffer;

    for (i = 0; i < (bytenbr / 4); i++)
    {
      sram_drvwrite(addr, *data++);
      addr = addr + 4;
    }

  }
  else
  {
    addr = ALLOC_START_ADDR + blockstr * BLOCK_SIZE;
    data = (uint32_t *)buffer;

    for (i = 0; i < (bytenbr / 4); i++)
    {
      sram_drvread(addr, data++);
      addr = addr + 4;
    }
  }
}
#endif

#ifndef TEST

/** 
 * @brief  low level writing consuctive memory blocks.
 *
 * @param buffer  pointer to data to operate on.
 * @param blockstr starting block of reading or writing data
 * @param blocknbr  nbr of consuctive blocks to write or read
 * @param write  1 write 0 read.

 * @return None.
 */
static void sram_onblock_op(void *buffer, uint8_t blockstr, uint8_t blocknbr, bool write)
{
  int i;
  uint32_t *source;
  uint32_t *destination;
  uint32_t bytenbr;

  if (write)
  {
    destination = (uint32_t *)ALLOC_START_ADDR + blockstr * BLOCK_SIZE;
    source = (uint32_t *)buffer;
  }
  else
  {
    source = (uint32_t *)ALLOC_START_ADDR + blockstr * BLOCK_SIZE;
    destination = (uint32_t *)buffer;
  }

  bytenbr = blocknbr * BLOCK_SIZE;
  
  for (i = 0; i < (bytenbr / 4); i++)
    {
      *destination++ = *source++;

    }
}
#endif

/**
 * @brief initialize block and allocation tables for the first time.
 *
 */
static void init_tables(void)
{
  int i;
  superblock_t *superblock = &gsuper_block;
  block_t *blocktable = gblock_table;
  alloc_t  *alloctable = galloc_table;

  /* Init block table*/
  /* Super block */

  blocktable[0].index =  superblock->spstrindex;
  blocktable[0].busy  =  1;
  blocktable[0].next  = -1;

  /* Block Table */

  blocktable[1].index =  superblock->btstrindex;
  blocktable[1].busy  =  1;
  blocktable[1].next  = -1;

  /* allocation table */

  blocktable[2].index =  superblock->alstrindex;
  blocktable[2].busy  =  1;
  blocktable[2].next  = -1;

  /* Data blocks */
  for (i = superblock->datastrindex; i < superblock->blentrynbr; i++)
  {
    blocktable[i].index =  i; /* Indexes */
    blocktable[i].busy  =  0; /* Not busy */
    blocktable[i].next  = -1; /* Points to nothing*/
  }

  sram_onblock_op(blocktable, superblock->btstrindex, 1, ALLOC_OP_WRITE);

  /* Initialize allocation table to zero */
  for (i = 0; i < superblock->alentrynbr; i++)
  {
    alloctable[i].index = i;
    alloctable[i].start = 0;
    alloctable[i].len   = 0;
    alloctable[i].used  = 0;
  }

  sram_onblock_op(alloctable, superblock->alstrindex, 1, ALLOC_OP_WRITE);
}

/**
 * @brief Add one entry to the super block and update the size
 *
 * @param superblock The superblock
 * @param blknbr the block numbers to update
 */
static void update_alloc_superblock(superblock_t *superblock, uint8_t blkbnr)
{
  superblock->allocnbr     += 1; /* No allocation yet. */
  superblock->usedblocknbr+= blkbnr;
  superblock->freeblocknbr = DATA_BLKNBR - superblock->usedblocknbr;
}

/**
 * @brief Remove one entry from the super block and update the size
 * @param superblock The superblock
 * @param blknbr the block numbers to remove
 */
static void update_free_superblock(superblock_t *superblock, uint8_t blkbnr)
{
  superblock->allocnbr     = superblock->allocnbr - 1; /* No allocation yet. */
  superblock->usedblocknbr = superblock->usedblocknbr - blkbnr;
  superblock->freeblocknbr = DATA_BLKNBR - superblock->usedblocknbr;
}

/**
 * @brief Read from the Sram the 3 iblocks.
 */
static void read_iblocks_sram(superblock_t *superblock,
                          block_t *blocktable,
                          alloc_t *alloctable)
{
  /* Read super block */
  sram_onblock_op(superblock, 0, 1, ALLOC_OP_READ);

  /* Read the block table */
  sram_onblock_op(blocktable, superblock->btstrindex, 1, ALLOC_OP_READ);

  /* Read the allocation table */
  sram_onblock_op(alloctable, superblock->alstrindex, 1, ALLOC_OP_READ);
}

/**
 * @brief Write and flush to the Sram the 3 iblocks.
 */
static void sync_iblocks_sram(superblock_t *superblock,
                          block_t *blocktable,
                          alloc_t *alloctable)
{
  /* Write super block */
  sram_onblock_op(superblock, 0, 1, ALLOC_OP_WRITE);

  /* Write the block table */
  sram_onblock_op(blocktable, superblock->btstrindex, 1, ALLOC_OP_WRITE);

  /* Write the allocation table */
  sram_onblock_op(alloctable, superblock->alstrindex, 1, ALLOC_OP_WRITE);
}

/**
 * @brief test if an allocation index is already used
 * 
 * @param alloc_id  allocation index
 *
 * @return 0 (ALLOC_OK), or a negative error code.
 */
static int isallocidvalid(int alloc_id)
{
  superblock_t *superblock = &gsuper_block;

  if ((alloc_id > superblock->alentrynbr) || (alloc_id < 0))
  {
    return ERR_ALLOC_ID_INVALID;
  }
  else
  {
    return ALLOC_OK;
  }
}

/**
 * @brief write or read to the allocation space a buffer of length len
 *
 * @param alloc_id  index of allocation
 * @param pos  byte offset of the allocation where to start operating on data
 * @param buffer  pointer to a data structure, array or single data.
 * @param len  length in bytes of the data operated on.
 * @param write  1 Write, 0 Read
 * 
 * @return 0 (ALLOC_OK) or negativ number with error code. see error enum
 */
static int sram_alloc_op(int alloc_id, uint32_t pos, void *buffer, uint32_t len, bool write)
{
  int i;
  int size;
  
  superblock_t *superblock = &gsuper_block;
  block_t      *blocktable = gblock_table;
  alloc_t      *alloctable = galloc_table;

  uint32_t blockoffset;
  uint32_t posoffset;
  uint32_t nbrblocks;
  uint32_t blocktrim;
  uint32_t trim;
  uint32_t bufflen;
  uint32_t lastlen;
  uint8_t *localbuffer = local_buffer;
  uint32_t nextblock;

  /* This also checks if id is allocated. */
  size = sram_getalloc_size(alloc_id);
  
  if(size < 0)
  {
    return size;
  }

  /* check whether position is in the range of writing. */
  if ((len + pos) > size)
  {
    /* length of data exceed allocated space */
    return ERR_ALLOC_RW_EXCEED;
  }

  /* Determine how many blocks are to be written */
  blockoffset = (pos / BLOCK_SIZE);
  posoffset = (pos % BLOCK_SIZE);
  nbrblocks = ((posoffset + len) / BLOCK_SIZE) + (((posoffset + len) % BLOCK_SIZE) > 0);

  if ((len + pos) > BLOCK_SIZE)
  {
    blocktrim = (BLOCK_SIZE - ((len + pos) % BLOCK_SIZE)) % BLOCK_SIZE;
  }
  else
  {
    blocktrim = BLOCK_SIZE - len - pos;
  }

  /* first block to operate. */
  nextblock = alloctable[alloc_id].start;

  /* Offset from the first block. */
  for (i = blockoffset; i > 0; i--)
  {
    nextblock = blocktable[nextblock].next;
  }
  
  dbg_msg("%s :blockoffset %d, firstposoffset %d, nbrblockstowrite %d, trim %d\n",
           ((write)?"Write" : "Read"), blockoffset, posoffset, nbrblocks, blocktrim);

  trim = 0;
  lastlen = 0;

  for (i = nbrblocks; i > 0; i--)
  {
    /* Read the buffer locally. */
    sram_onblock_op(localbuffer, nextblock, 1, ALLOC_OP_READ);

    if (i == 1)
    {
      trim = blocktrim;
    }

    bufflen = BLOCK_SIZE - posoffset - trim;

    dbg_msg("blocks number = %d, len %d, off %d\n", nextblock, bufflen, posoffset);

    if (write)
    {
      /* OverWrite locally by buffer data. */
      memcpy(localbuffer + posoffset, buffer + lastlen, bufflen);

      /* Write the block back */
      sram_onblock_op(localbuffer, nextblock, 1, ALLOC_OP_WRITE);
    }
    else
    {
      /* copy to read buffer.*/
      memcpy(buffer + lastlen, localbuffer + posoffset, bufflen);
    }

    /* Copy to the buffer from last length. */
    lastlen += bufflen;

    /* Jump to next block */
    nextblock = blocktable[nextblock].next;

    /* offset serves only at the begining. */
    posoffset = 0;
  }

  return ALLOC_OK;
}

/**
 * @brief Mount the allocation system in the SRAM
 */
int mount_allocsystem(void)
{
  /* Initialize block table. */
  superblock_t *superblock = &gsuper_block;
  block_t      *blocktable = gblock_table;
  alloc_t      *alloctable = galloc_table;
  
  /* SRAM location should never be accessed directly always use functions. */
 
  sram_onblock_op(superblock, 0, 1, ALLOC_OP_READ);

  if (superblock->magicnbr != (uint32_t)ALLOC_MAGIC_NBR)
    {
      /* SRAM ALLOCATION SYSTEM is not present */
      superblock->magicnbr     = (uint32_t)ALLOC_MAGIC_NBR;
      superblock->bsize        = BLOCK_SIZE;
      superblock->blocknbr     = BLOCK_NUMBER;
      superblock->usedblocknbr = 0;
      superblock->freeblocknbr = DATA_BLKNBR;
      superblock->spblocknbr   = SUPER_BLKNBR;  
      superblock->btblocknbr   = BLOCK_TABLE_BLKNBR;  
      superblock->alblocknbr   = ALLOC_TABLE_BLKNBR;  
      superblock->datablocknbr = DATA_BLKNBR;/* the reste of blocks */
      superblock->spstrindex   = SUPER_STR_INDX;
      superblock->btstrindex   = BLOCK_TABLE_STR_INDX;
      superblock->alstrindex   = ALLOC_TABLE_STR_INDX; 
      superblock->datastrindex = DATA_STR_INDX;
      superblock->blentrynbr   = BLOCK_TABLE_ROW_NBR;
      superblock->alentrynbr   = ALLOC_TABLE_ROW_NBR;
      superblock->allocnbr     = 0; /* No allocation yet. */

      dbg_msg("No Alloc system found. Allocating...\n");
      dbg_msg("magicnbr : %08X, blocknbr : %d, entrynbrs: %d\n",
      superblock->magicnbr, superblock->blocknbr, superblock->alentrynbr);

      sram_onblock_op(superblock, 0, 1, ALLOC_OP_WRITE);

      init_tables();
    }
  else
    {
      /* If system already mounted load iblocks locally. */
      read_iblocks_sram(superblock, blocktable, alloctable);
    }

  return ALLOC_OK;
}

int sram_block_malloc(int alloc_id, size_t bytenbr)
{
  int i, j;
  int lastindex = -1;
  superblock_t *superblock = &gsuper_block;
  block_t      *blocktable = gblock_table;
  alloc_t      *alloctable = galloc_table;
  
  uint8_t blocknbr;
  int firstblock = 0;
  int blockcount;

  if (bytenbr < 0)
  {
    /* bytenbr invalid */
    return ERR_ALLOC_INVALID_BYTENBR;
  }
  else if (bytenbr <= BLOCK_SIZE)
  {
    blocknbr = 1;
  }
  else
  {
    blocknbr = (bytenbr / BLOCK_SIZE) + ((bytenbr%BLOCK_SIZE) > 0);
  }

  /* Check if system is mounted properly. */
  if (superblock->magicnbr != (uint32_t)ALLOC_MAGIC_NBR)
    {
      return ERR_ALLOC_SYS_NOTFOUND;
    }

  if (superblock->allocnbr > ALLOC_TABLE_ROW_NBR)
  {
    /* Allocation table full. */
    return ERR_ALLOC_TABLE_FULL;
  }

  if (superblock->freeblocknbr < blocknbr)
  {
    /* Allocation failed not enough blocks. */
    return ERR_BLOCK_TABLE_FULL;
  }

  if (alloctable[alloc_id].used == 1)
  {
    return ERR_ALLOC_USED;
  }

  if ((alloc_id > superblock->alentrynbr) || (alloc_id < 0))
  {
    return ERR_ALLOC_ID_INVALID;
  }

  /* Search for free blocks */
  blockcount = blocknbr;
  for (i = superblock->datastrindex; i < superblock->blentrynbr; i++)
  { 
    if (blocktable[i].busy != (uint8_t)1)
    {
      /* Allocation found. */
      if (!firstblock)
      {
        firstblock = i;
      }
      else
      {
        blocktable[lastindex].next = i;
      }

      blocktable[i].busy = 1;

      blockcount--;
      if (blockcount == 0)
      {
        blocktable[i].next = -1;

        alloctable[alloc_id].used = 1;
        alloctable[alloc_id].start = firstblock;
        alloctable[alloc_id].len = blocknbr;

        update_alloc_superblock(superblock, blocknbr);

        sync_iblocks_sram(superblock, blocktable, alloctable);

        break;
      }
      lastindex = i;
    }
  }
  if (i > superblock->blentrynbr)
  {
    return ERR_ALLOC_BLOCKS_FULL;
  }

  if (blockcount != 0)
  {
    /* Allocation interrupted.*/
    return ERR_ALLOC_FAILED;
  }

  return ALLOC_OK;
}

int sram_block_free(int alloc_id)
{
  int i;
  int lastindex = -1;
  superblock_t *superblock = &gsuper_block;
  block_t      *blocktable = gblock_table;
  alloc_t      *alloctable = galloc_table;
  
  uint8_t blocknbr;
  int firstblock = 0;
  int nextblock = 0;

  /* Check if system is mounted properly. */
  if (superblock->magicnbr != (uint32_t)ALLOC_MAGIC_NBR)
    {
      return ERR_ALLOC_SYS_NOTFOUND;
    }

  /* Get the file ID from handle */

  alloctable[alloc_id].used = 0;
  firstblock = alloctable[alloc_id].start;
  blocknbr = alloctable[alloc_id].len;

  /* Use this first block to go through all the blocks*/
  for (i = firstblock; i != -1; i = nextblock)
  {
    blocktable[i].busy = 0;
    nextblock = blocktable[i].next;
    blocktable[i].next = -1;
  }

  update_free_superblock(superblock, blocknbr);
  
  sync_iblocks_sram(superblock, blocktable, alloctable);

  return ALLOC_OK;
}

int sram_alloc_write(int alloc_id, uint32_t pos, void *buffer, uint32_t len)
{
  return sram_alloc_op(alloc_id, pos, buffer, len, ALLOC_OP_WRITE);
}

/* Read a maximum of 512 byte*/
int sram_alloc_read(int alloc_id, uint32_t pos, void *buffer, uint32_t len)
{
  return sram_alloc_op(alloc_id, pos, buffer, len, ALLOC_OP_READ);
}

int sram_getentrynbr(void)
{
  superblock_t *superblock = &gsuper_block;

  return superblock->allocnbr;
}

int sram_gettotalblocks(void)
{
  superblock_t *superblock = &gsuper_block;

  return superblock->datablocknbr;
}

int sram_getblocksize(void)
{
  superblock_t *superblock = &gsuper_block;

  return superblock->bsize;
}
int sram_getfreeblocks(void)
{
  superblock_t *superblock = &gsuper_block;

  return superblock->freeblocknbr;
}

int sram_getusedblocks(void)
{
  superblock_t *superblock = &gsuper_block;

  return superblock->usedblocknbr;
}

/* Get size of allocation in bytes */

int sram_getalloc_size(int alloc_id)
{
  alloc_t      *alloctable = galloc_table;

  int ret ;

  ret =  isallocidvalid(alloc_id) ;
  if (ret < 0)
  {
    return ret;
  }

  return (alloctable[alloc_id].len * BLOCK_SIZE);
}

/* Get of is used */
int sram_getalloc_valid(int alloc_id)
{
  alloc_t      *alloctable = galloc_table;

  int ret ;

  ret =  isallocidvalid(alloc_id) ;
  if (ret < 0)
  {
    return ret;
  }

  return alloctable[alloc_id].used;
}