#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define TEST

#define ALLOC_OP_READ  0
#define ALLOC_OP_WRITE 1

#define BLOCK_DATA_STRUCT_LEN  4 /* Length of block in byte */
#define ALLOC_DATA_STRUCT_LEN  4 /* Length of alloc row in byte */

#define BLOCK_SIZE    512 /* BLOCK size in bytes */
#define SRAM_SIZE     (64 * 1024) /* Total size of sram in bytes */
#define BLOCK_NUMBER  (SRAM_SIZE / BLOCK_SIZE) /* TOTAL BLOCKS nbr in sytsem */

#define BLOCK_TABLE_ROW_NBR (BLOCK_SIZE / BLOCK_DATA_STRUCT_LEN )
#define ALLOC_TABLE_ROW_NBR (BLOCK_SIZE / ALLOC_DATA_STRUCT_LEN)

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

#define ALLOC_MAGIC_NBR  0xDE0FA751
#define ALLOC_START_ADDR 0x00000000
#define ALLOC_SUPER_BLCOK_ADDR  (ALLOC_START_ADDR + 0 * BLOCK_SIZE) /* Block.0 */
#define ALLOC_BLOCK_TABLE_ADDR  (ALLOC_START_ADDR + 1 * BLOCK_SIZE) /* Block.1 */
#define ALLOC_FILE_TABLE_ADDR   (ALLOC_START_ADDR + 2 * BLOCK_SIZE) /* Block.2 */

#define dbg_msg(format, ...)     printf(format, ##__VA_ARGS__)

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

struct sram_t
{
  uint8_t data[SRAM_SIZE];
  uint32_t addr[SRAM_SIZE];
  
}sram;

typedef struct 
{
  uint8_t index; /* 0-255 indexing */
  uint8_t busy;  /* 1: busy, 0: free*/
  int16_t  next; /* Next index reserved (-1 of non) Warning (-127, 128) range. */
}block_t;
/* /!\ It is essential for this struct to not exceed 32bit */
 


/* Blocks table */
/* This table holds data about each block and relation with next block */
static block_t gblock_table[BLOCK_TABLE_ROW_NBR];

typedef struct 
{
  uint8_t index;    /* Id of the allocation space */
  uint8_t start; /* Starting index */
  uint8_t len;   /* nbr of blocks reserved */
  uint8_t used; /* Is this entry used or not. */
}alloc_t;
/* /!\ It is essential for this struct to not exceed 32bit */

/* Allocation table */
/* This table holds data of allocated spaces,*/
static alloc_t galloc_table[ALLOC_TABLE_ROW_NBR];

/* Super block hold data about the allocation system. */
typedef struct
{
  uint32_t  magicnbr;    /* Special nbre to check whether a system is mounted. */
  uint32_t  bsize;       /* Size of one block in bytes */
  uint32_t  blocknbr;   /* total block nbr in sys */
  uint32_t  usedblocknbr;   /* total used data blocks nbr in sys */
  uint32_t  freeblocknbr;   /* total free data blocks nbr in sys */
  uint32_t  spblocknbr;   /* nbr blocks reserve for super block */
  uint32_t  btblocknbr;  /* nbr block reserved for block table */
  uint32_t  alblocknbr;  /* nbr block reserved for allocation table*/
  uint32_t  datablocknbr;  /* nbr block reserved for data */
  uint32_t  spstrindex; /* super block start index. */
  uint32_t  btstrindex; /* Block table start index. */
  uint32_t  alstrindex; /* alloc table start index. */
  uint32_t  datastrindex; /* data start index. */
  uint32_t  blentrynbr; /*  nbr of rows in Block table. */
  uint32_t  alentrynbr; /*  nbr of rows in Alloc table. */
  uint32_t  allocnbr; /* nbr of allocations made */
}superblock_t;
/* /!\ It is essential for this struct to not exceed BLOCK_SIZE (512byte) */


/* handle to identify an allocation. */
typedef struct
{
  uint8_t id;   /* ID of file */
  uint32_t size; /* length of allocation in bytes */
}sram_alloc_t;  


static superblock_t gsuper_block;


static bool isaddr_valid(uint32_t addr)
{
  return !(addr%4 > 0);
}

/* Simulate Writing in the sram */
static int writesram(uint32_t addr, uint32_t data)
{

  if (isaddr_valid(addr))
  {
    sram.data[addr + 0] = (data >> 24) & 0xFF;
    sram.data[addr + 1] = (data >> 16) & 0xFF;
    sram.data[addr + 2] = (data >> 8) & 0xFF;
    sram.data[addr + 3] = (data) & 0xFF;
    return 0;
  }
  else
  {
    dbg_msg("Invalid Addr.\n");
    return 1;
  }
}

/* Simulate Reading from the sram */
static int readsram(uint32_t addr, uint32_t *data)
{
  uint32_t ldata;

  if (isaddr_valid(addr))
  {
    ldata = ((sram.data[addr + 0] << 24) & 0xFFFFFFFF) +
            ((sram.data[addr + 1] << 16) & 0xFFFFFFFF) +
            ((sram.data[addr + 2] << 8) & 0xFFFFFFFF) +
            ((sram.data[addr + 3]) & 0xFFFFFFFF);

    *data = ldata;
    return 0;
  }
  else
  {
    dbg_msg("Invalid Addr.\n");
    return 1;
  }
}

static int sram_onblock_op(void *buffer, uint8_t blockstr, uint8_t blocknbr, bool write)
{
  int i;
  #ifndef TEST 
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
  #else
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
      writesram(addr, *data++);
      addr = addr + 4;
    }

  }
  else
  {
    addr = ALLOC_START_ADDR + blockstr * BLOCK_SIZE;
    data = (uint32_t *)buffer;

    for (i = 0; i < (bytenbr / 4); i++)
    {
      readsram(addr, data++);
      addr = addr + 4;
    }
  }

  

  #endif
}

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

static int mount_allocsystem(void)
{
  /* Initialize block table. */
  /* Check the fist block for block table if not initialize new one. */

  /* Points this to the SRAM Address */
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

void init_sram(void)
{
  for (int i = 0; i < SRAM_SIZE; ++i)
  {
    sram.addr[i] = i;
    sram.data[i] = 0xFF; 
  }

  dbg_msg("Sram starting from index zero\n");
}

static void dump_sram(int blocknbr)
{
  int i;

  dbg_msg("SRAM memory dump : \n");
  uint32_t addr = ALLOC_START_ADDR + blocknbr * BLOCK_SIZE;

  for (i = addr; i < (addr + BLOCK_SIZE); i+=4)
  {
    dbg_msg(" %08X | %02X%02X%02X%02X\n",
             sram.addr[i],
             sram.data[i],
             sram.data[i + 1],
             sram.data[i + 2],
             sram.data[i + 3]
             );
  }
}
void sram_unit_test(void)
{
  uint32_t addr =  0;
  uint32_t data = 0x01234567;
  uint32_t rdata = 0x00000000;

  writesram(addr, data);

  dump_sram(1);

  readsram(addr, &rdata);
  dbg_msg("Read data : %08X\n", rdata);

  data = 0xDE07C0DE;
  rdata = 0x00000000;
  writesram(addr + 4, data);
  dump_sram(2);

  readsram(addr + 4, &rdata);
  dbg_msg("Read data : %08X\n", rdata);
}

void sram_block_op_test(void)
{
  uint8_t blockbuffer[512];

  uint8_t buffer[256];
  uint8_t rbuffer[256];

  for (int i = 0; i < 256; i++)
  {
    buffer[i] = i;
  }

  dbg_msg("Write data : \n");
  for (int i = 0; i < 128; ++i)
  {
    dbg_msg("%02X ", buffer[i]);
    /* code */
  }

  memcpy(blockbuffer, buffer, 256);
  sram_onblock_op(blockbuffer, 0, 1, ALLOC_OP_WRITE);
  
  dump_sram(128);

  sram_onblock_op(blockbuffer, 0, 1, ALLOC_OP_READ);
  memcpy(rbuffer, blockbuffer, 256);

  dbg_msg("read data : \n");
  for (int i = 0; i < 128; ++i)
  {
    dbg_msg("%02X ", rbuffer[i]);
    /* code */
  }
}

int main(int argc, char const *argv[])
{
  sram_alloc_t shandle;
  sram_alloc_t shandle2;
  sram_alloc_t shandle3;
  int ret;
  superblock_t *superblock = &gsuper_block;
  block_t      *blocktable = gblock_table ;
  alloc_t      *alloctable = galloc_table ;

  init_sram();
  mount_allocsystem();

  if (sram_block_malloc(&shandle, 1024) == 0)
  {
    dbg_msg("Allocation sucess id %d, size %d\n", shandle.id, shandle.size);
  }

  dbg_msg("Super blocknbr %d\n", superblock->allocnbr);


  if (sram_block_malloc(&shandle2, 512) == 0)
  {
    dbg_msg("Allocation sucess id %d, size %d\n", shandle2.id, shandle2.size);
  }
  
  read_iblocks_sram(superblock, blocktable, alloctable);
  //dump_sram(1);

  dbg_msg("Super freeblocknbr %d\n", superblock->freeblocknbr);

  if(sram_block_free(&shandle) == 0)
  {
    dbg_msg("Block freed\n");
  }
  read_iblocks_sram(superblock, blocktable, alloctable);

  dbg_msg("Super freeblocknbr %d\n", superblock->freeblocknbr);
  dbg_msg("Super allocnbr %d\n", superblock->allocnbr);

  if (sram_block_malloc(&shandle3, 64*512) == 0)
  {
    dbg_msg("Allocation sucess id %d, size %d\n", shandle3.id, shandle3.size);
  }
  
  dbg_msg("Super freeblocknbr %d\n", superblock->freeblocknbr);

  //dump_sram(1);
  return 0;
}