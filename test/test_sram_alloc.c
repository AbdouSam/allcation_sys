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

#define LEN (1)
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

#if 1
typedef struct
{
  uint8_t id;
  uint32_t stuff;
  uint16_t others;
  uint64_t longer;
}test_t;

void test_sram_alloc_WriteReadStructures(void)
{
  int id0 = 0;
  int size0 = 5*1024; /* 2 blocks */
  int pos =  0;

  test_t wbuffer;
  test_t rbuffer;

  int ret;

  srand(1);

  wbuffer.id = 12;
  wbuffer.stuff = 0xDEADBEAF;
  wbuffer.others = 0x0077;
  wbuffer.longer=  0xDE01C0DECAFEBABE;

  rbuffer.id = 0;
  rbuffer.stuff = 0;
  rbuffer.others = 0;
  rbuffer.longer = 0;

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

  ret = sram_alloc_write(id0, pos, &wbuffer, sizeof(wbuffer));
  if (ret >= 0)
  {
    dbg_msg("Write sucess\n");
  }
  else
  {
    dbg_msg("Write Failed Err %d\n", ret);
  }

  ret = sram_alloc_read(id0, pos, &rbuffer, sizeof(rbuffer));
  if (ret >= 0)
  {
    dbg_msg("Read sucess\n");
  }
  else
  {
    dbg_msg("Read Failed Err %d\n", ret);
  }

  TEST_ASSERT_EQUAL_UINT8(wbuffer.id, rbuffer.id);
  TEST_ASSERT_EQUAL_UINT32(wbuffer.stuff, rbuffer.stuff);
  TEST_ASSERT_EQUAL_UINT16(wbuffer.others, rbuffer.others);
  TEST_ASSERT_EQUAL_UINT64(wbuffer.longer, rbuffer.longer);
}
#endif
