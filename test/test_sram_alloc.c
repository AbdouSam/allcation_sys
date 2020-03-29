#include "unity.h"

#include "sram_alloc.h"
#include "sram_drv.h"
#include "debug.h"
#include "config.h"

void setUp(void)
{
  /* Initialize SRAM to simulate the hardware */
  init_sram();
  /* mount the file system. */
  
}

void tearDown(void)
{
}

#if 0
void test_sram_alloc_GlobalTesting(void)
{
  sram_alloc_t shandle;
  sram_alloc_t shandle2;
  sram_alloc_t shandle3;

  int ret;
  
  mount_allocsystem();

  if (sram_block_malloc(&shandle, 1024) == 0)
  {
    dbg_msg("Allocation sucess id %d, size %d\n", shandle.id, shandle.size);
  }

  dbg_msg("Super blocknbr %d\n", sram_getentrynbr());


  if (sram_block_malloc(&shandle2, 512) == 0)
  {
    dbg_msg("Allocation sucess id %d, size %d\n", shandle2.id, shandle2.size);
  }
  
  //dump_sram(1);

  dbg_msg("Super freeblocknbr %d\n", sram_getfreeblocks());

  if(sram_block_free(&shandle) == 0)
  {
    dbg_msg("Block freed\n");
  }

  dbg_msg("Super freeblocknbr %d\n", sram_getfreeblocks());
  dbg_msg("Super allocnbr %d\n", sram_getentrynbr());

  if (sram_block_malloc(&shandle3, 64*512) == 0)
  {
    dbg_msg("Allocation sucess id %d, size %d\n", shandle3.id, shandle3.size);
  }
  
  dbg_msg("Super freeblocknbr %d\n", sram_getfreeblocks());

  //dump_sram(1);
}

#endif

#if 1
void test_sram_alloc_BlockOP(void)
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
  }
  dbg_msg("\n");

  memcpy(blockbuffer, buffer, 256);
  sram_onblock_op(blockbuffer, 0, 1, ALLOC_OP_WRITE);
  
  dump_sram(0);

  sram_onblock_op(blockbuffer, 0, 1, ALLOC_OP_READ);
  memcpy(rbuffer, blockbuffer, 256);

  dbg_msg("read data : \n");
  for (int i = 0; i < 128; ++i)
  {
    dbg_msg("%02X ", rbuffer[i]);
  }
  dbg_msg("\n");
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