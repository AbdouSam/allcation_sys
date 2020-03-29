#ifndef CONFIG_H
#define CONFIG_H


#define BLOCK_SIZE    512            /* BLOCK size in bytes */
#define SRAM_SIZE     (64 * 1024)    /* Total size of sram in bytes */
#define ALLOC_START_ADDR 0x00000000

#define ALLOC_OP_READ  0
#define ALLOC_OP_WRITE 1

#endif /* CONFIG_H */