#include "sram_alloc.h"
#include "debug.h"
#include "config.h"

#define TEST

#define BLOCK_DATA_STRUCT_LEN  4 /* Length of block in byte */
#define ALLOC_DATA_STRUCT_LEN  4 /* Length of alloc row in byte */
#define BLOCK_NUMBER          (SRAM_SIZE / BLOCK_SIZE) /* TOTAL BLOCKS nbr in sytsem */
#define BLOCK_TABLE_ROW_NBR   (BLOCK_SIZE / BLOCK_DATA_STRUCT_LEN )
#define ALLOC_TABLE_ROW_NBR   (BLOCK_SIZE / ALLOC_DATA_STRUCT_LEN)

/* Size in byte / 4 to get 32bit addresses
 * with 512/ 4 = 128 Address to hold data about 128 allocation spaces. */

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
 * TODO :
 * - Limitation is the blocks table is limited to 128 block addressing only.
 *   We can make that more flexiblewe can increase the block size more thn 512
 *   but this will create a problem, one entry in block table should be 4 byte
 *   the index is 1 byte (256 max address)
 */

/* a block entry, holds data about a block */
typedef struct 
{
  uint8_t index; /* 0-255 indexing */
  uint8_t busy;  /* 1: busy, 0: free*/
  int16_t next;  /* Next index reserved (-1 of non) Warning (-127, 128) range. */
}block_t;
/* /!\ It is essential for this struct to not exceed 32bit */
 
 /* allocation entry, hold data about an allocation */
typedef struct 
{
  uint8_t index; /* Id of the allocation space */
  uint8_t start; /* Starting index */
  uint8_t len;   /* nbr of blocks reserved */
  uint8_t used;  /* Is this entry used or not. */
}alloc_t;
/* /!\ It is essential for this struct to not exceed 32bit */


/* Super block hold data about the allocation system. */
typedef struct
{
  uint32_t  magicnbr;     /* Special nbre to check whether a system is mounted. */
  uint32_t  bsize;        /* Size of one block in bytes */
  uint32_t  blocknbr;     /* total block nbr in sys */
  uint32_t  usedblocknbr; /* total used data blocks nbr in sys */
  uint32_t  freeblocknbr; /* total free data blocks nbr in sys */
  uint32_t  spblocknbr;   /* nbr blocks reserve for super block */
  uint32_t  btblocknbr;   /* nbr block reserved for block table */
  uint32_t  alblocknbr;   /* nbr block reserved for allocation table*/
  uint32_t  datablocknbr; /* nbr block reserved for data */
  uint32_t  spstrindex;   /* super block start index. */
  uint32_t  btstrindex;   /* Block table start index. */
  uint32_t  alstrindex;   /* alloc table start index. */
  uint32_t  datastrindex; /* data start index. */
  uint32_t  blentrynbr;   /* nbr of rows in Block table. */
  uint32_t  alentrynbr;   /* nbr of rows in Alloc table. */
  uint32_t  allocnbr;     /* nbr of allocations made */
}superblock_t;
/* /!\ It is essential for this struct to not exceed BLOCK_SIZE (512byte) */

static block_t gblock_table[BLOCK_TABLE_ROW_NBR];
static alloc_t galloc_table[ALLOC_TABLE_ROW_NBR];
static superblock_t gsuper_block;

#ifdef TEST
int sram_onblock_op(void *buffer, uint8_t blockstr, uint8_t blocknbr, bool write)
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
int sram_onblock_op(void *buffer, uint8_t blockstr, uint8_t blocknbr, bool write)
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

/* Limitation of this function, super table and alloc tables should be of size 1. */
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

int mount_allocsystem(void)
{
  /* Initialize block table. */
  superblock_t *superblock = &gsuper_block;
 
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
}

/**
 * @brief Add one entry to the super block and update the size
 */
static void update_alloc_superblock(superblock_t *superblock, uint8_t blkbnr)
{
  superblock->allocnbr     += 1; /* No allocation yet. */
  superblock->usedblocknbr+= blkbnr;
  superblock->freeblocknbr = DATA_BLKNBR - superblock->usedblocknbr;
}

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

/* This function needs a LOCK */

/* Note The minimum Allocation block is 512byte */
int sram_block_malloc(sram_alloc_t *handle, size_t bytenbr)
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

  read_iblocks_sram(superblock, blocktable, alloctable);

  /* Check if system is mounted properly. */
  if (superblock->magicnbr != (uint32_t)ALLOC_MAGIC_NBR)
    {
      return ERR_ALLOC_SYS;
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

        for (j = 0; j < superblock->alentrynbr; j++)
        {
          if (!alloctable[j].used)
          {
            alloctable[j].used = 1;
            alloctable[j].start = firstblock;
            alloctable[j].len = blocknbr;
            break;
          }
        }

        if (j > superblock->alentrynbr)
        {
          return ERR_ALLOC_FAILED;
        }

        handle->id = alloctable[j].index;
        handle->size = blocknbr * superblock->bsize; /* Length in bytes */

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

int sram_block_free(sram_alloc_t *handle)
{
  int i;
  int lastindex = -1;
  superblock_t *superblock = &gsuper_block;
  block_t      *blocktable = gblock_table;
  alloc_t      *alloctable = galloc_table;
  
  uint8_t blocknbr;
  int firstblock = 0;
  int nextblock = 0;

  read_iblocks_sram(superblock, blocktable, alloctable);

  /* Check if system is mounted properly. */
  if (superblock->magicnbr != (uint32_t)ALLOC_MAGIC_NBR)
    {
      return ERR_ALLOC_SYS;
    }

  /* Get the file ID from handle */
  if (handle != NULL)
  {
    alloctable[handle->id].used = 0;
    firstblock = alloctable[handle->id].start;
    blocknbr = alloctable[handle->id].len;

    /* Use this first block to go through all the blocks*/
    for (i = firstblock; i != -1; i = nextblock)
    {
      blocktable[i].busy = 0;
      nextblock = blocktable[i].next;
      blocktable[i].next = -1;
    }

    update_free_superblock(superblock, blocknbr);
    

    sync_iblocks_sram(superblock, blocktable, alloctable);

    /* Set the Handle to NULL after finish. */    
    handle = NULL;

    return ALLOC_OK;
  }
  else
  {
    return ERR_INVALID_ALLOC_HANDLE;
  }
}
static uint8_t alloc_buffer[BLOCK_SIZE];

void sram_alloc_write(sram_alloc_t *handle, uint32_t pos, void *buffer, uint32_t len)
{
  /* Get information from handle */
  uint32_t id = handle->id;
  uint32_t size = handle->size;

  /* check whether position is in the range of writing. */
  if ((len + pos) > size)
  {
    /* length of data exceed allocated space */
    return -1;
  }

  /* Determine how many blocks are to be written */
  uint32_t blkoffset = (pos / BLOCK_SIZE);
  uint32_t posoffset = (pos % BLOCK_SIZE)

  uint32_t nbrblocks = ((posoffset + len) / BLOCK_SIZE) + (((posoffset + len) % BLOCK_SIZE) > 0);

  /* Read sram before writing. loop through, */
  for (i = nbrblocks; i < 0; i--)
  {
    /* Read first block, whichis the startblock of the allocation plus the block offset */
    /* Get it into the buffer locally */

    /* Make only the necessary changes locally, meaning : memset from posoffset */
    /* Write the block back*/

    /* Read the next block,  */
    /* Check how many bytes are left if > block_size write fully if bytes < block_size memset only the changes bytes. */
    /* Write to sram */
  }
}

int sram_getentrynbr(void)
{
  superblock_t *superblock = &gsuper_block;

  return superblock->allocnbr;
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