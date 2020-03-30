#include "unity.h"

#include "sram_alloc.h"
#include "sram_drv.h"
#include "debug.h"
#include "config.h"

void setUp(void)
{
  /* Initialize SRAM to simulate the hardware */
  init_sram();
  
}

void tearDown(void)
{
}

#if 0
void test_sram_alloc_MallocFree(void)
{
  int id0 = 0;
  int size0 = 1024; /* 2 blocks */

  int id1 = 10;
  int size1 = 5 * 512; /* 5 blocks */

  int id2 = 45;
  int size2 = 7 * 255; /* 3 and half blocks */

  int totalblocks;
  int blocksize;
  /* Mounting system */

  mount_allocsystem();
  totalblocks = sram_gettotalblocks();
  blocksize   = sram_getblocksize();

  /* First allocation */
  if (sram_block_malloc(id0, size0) == 0)
  {
    dbg_msg("Allocation sucess id %d.\n", id0);
  }
  else
  {
    dbg_msg("Allocation id %d Failed.\n", id0);
  }

  /* One Entry allocated */
  TEST_ASSERT_EQUAL(1 , sram_getentrynbr());

  /* USed blocks*/
  TEST_ASSERT_EQUAL(sram_getalloc_size(id0) / blocksize , sram_getusedblocks());

  if (sram_block_malloc(id1, size1) == 0)
  {
    dbg_msg("Allocation sucess id %d\n", id1);
  }
  else
  {
    dbg_msg("Allocation id %d Failed\n", id1);
  }
  
  /* Two entries allocated. */
  TEST_ASSERT_EQUAL(2 , sram_getentrynbr());

  /* Used blocks */
  TEST_ASSERT_EQUAL((sram_getalloc_size(id0) + sram_getalloc_size(id1))/ blocksize,
                     sram_getusedblocks());

  if(sram_block_free(id0) == 0)
  {
    dbg_msg("Block id %d freed\n", id0);
  }

  /* One Entry Left */
  TEST_ASSERT_EQUAL(1 , sram_getentrynbr());

  /* we freed id 0, the only used space is id1 */
  TEST_ASSERT_EQUAL(totalblocks - (sram_getalloc_size(id1) / blocksize),
                     sram_getfreeblocks());

  /* A third allocation */
  if (sram_block_malloc(id2, size2) == 0)
  {
    dbg_msg("Allocation sucess id %d\n", id2);
  }
  else
  {
    dbg_msg("Allocation id %d Failed\n", id2);
  }
  
   /* One Entry Left */
  TEST_ASSERT_EQUAL(2 , sram_getentrynbr());

  /* Used blocks now is id1 and id2 */
  TEST_ASSERT_EQUAL((sram_getalloc_size(id1) + sram_getalloc_size(id2))/ blocksize,
                     sram_getusedblocks());

  //dump_sram(1);
}

#endif

#if 0
void test_sram_alloc_BlockOP(void)
{
  uint8_t blockbuffer[512];

  uint8_t wbuffer[256];
  uint8_t rbuffer[256];

  int starting_block = 0;
  int nbr_consuctive_block = 1;

  for (int i = 0; i < 256; i++)
  {
    wbuffer[i] = i;
  }

  memcpy(blockbuffer, wbuffer, 256);
  sram_onblock_op(blockbuffer, starting_block, nbr_consuctive_block, ALLOC_OP_WRITE);
  
  //dump_sram(starting_block);

  sram_onblock_op(blockbuffer, starting_block, nbr_consuctive_block, ALLOC_OP_READ);
  memcpy(rbuffer, blockbuffer, 256);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(wbuffer, rbuffer, 256);
}
#endif

#if 0
void test_sram_alloc_SRAMReadWrite(void)
{
  uint32_t addr =  0;
  uint32_t data = 0x01234567;
  uint32_t rdata = 0x00000000;

  sram_drvwrite(addr, data);

  dump_sram(1);

  sram_drvread(addr, &rdata);
  dbg_msg("Read data : %08X\n", rdata);

  data = 0xDE07C0DE;
  rdata = 0x00000000;
  sram_drvwrite(addr + 4, data);
  dump_sram(2);

  sram_drvread(addr + 4, &rdata);
  dbg_msg("Read data : %08X\n", rdata);
}
#endif

#if 0
void test_sram_alloc_WritemultipleBlocks(void)
{
  int id0 = 0;
  int size0 = 2*1024; /* 2 blocks */
  int pos = 512 + 128;
  int len = 128;
  uint8_t wbuffer[128];
  int ret;

  for (int i = 0; i < len; i++)
  {
    wbuffer[i] = i;
  }

  mount_allocsystem();

  /* Allocation */
  ret = sram_block_malloc(id0, size0);

  if (ret == 0)
  {
    dbg_msg("Allocation sucess id %d.\n", id0);
  }
  else
  {
    dbg_msg("Allocation id %d Failed. Err %d\n", id0, ret);
  }

  ret = sram_alloc_write(id0, pos, wbuffer, len);
  if (ret >= 0)
  {
    dbg_msg("Write sucess\n");
  }
  else
  {
    dbg_msg("Write Failed Err %d\n", ret);
  }

  dump_sram(4);
}
#endif

#if 1

#define LEN (4*1024)
void test_sram_alloc_WriteReadmultipleBlocks(void)
{
  int id0 = 0;
  int size0 = 5*1024; /* 2 blocks */
  int pos = 256 + 128;
  int len = LEN;
  uint8_t wbuffer[LEN];
  uint8_t rbuffer[LEN];
  int ret;

  srand(1);

  for (int i = 0; i < len; i++)
  {
    wbuffer[i] = rand() % 0xFF;

    rbuffer[i] = 0xFF;

  }

  /* Mount system. */
  mount_allocsystem();

  /* Allocation */
  ret = sram_block_malloc(id0, size0);

  if (ret == 0)
  {
    dbg_msg("Allocation sucess id %d.\n", id0);
  }
  else
  {
    dbg_msg("Allocation id %d Failed. Err %d\n", id0, ret);
  }

  ret = sram_alloc_write(id0, pos, wbuffer, len);
  if (ret >= 0)
  {
    dbg_msg("Write sucess\n");
  }
  else
  {
    dbg_msg("Write Failed Err %d\n", ret);
  }

  ret = sram_alloc_read(id0, pos, rbuffer, len);
  if (ret >= 0)
  {
    dbg_msg("Read sucess\n");
  }
  else
  {
    dbg_msg("Read Failed Err %d\n", ret);
  }

  TEST_ASSERT_EQUAL_UINT8_ARRAY(wbuffer, rbuffer, len);
}
#endif

