#ifndef SRAM_DRV_H
#define SRAM_DRV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


void init_sram(void);
void dump_sram(int blocknbr);

int sram_drvwrite(uint32_t addr, uint32_t data);
int sram_drvread(uint32_t addr, uint32_t *data);


#endif // SRAM_DRV_H
