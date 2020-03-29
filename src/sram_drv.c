#include "sram_drv.h"
#include "debug.h"
#include "config.h"

struct sram_t
{
  uint8_t data[SRAM_SIZE];
  uint32_t addr[SRAM_SIZE];
};

static struct sram_t sram;

void init_sram(void)
{
  for (int i = 0; i < SRAM_SIZE; ++i)
  {
    sram.addr[i] = i;
    sram.data[i] = 0xFF; 
  }

  dbg_msg("Sram starting from index zero\n");
}

/* Dump the block data. */
void dump_sram(int blocknbr)
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

static bool isaddr_valid(uint32_t addr)
{
  return !(addr%4 > 0);
}

/* Simulate Writing in the sram */
int sram_drvwrite(uint32_t addr, uint32_t data)
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
int sram_drvread(uint32_t addr, uint32_t *data)
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