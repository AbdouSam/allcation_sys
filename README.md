# README

This is a Simple memory allocation system for external SRAM (32bit addressing).
The system is simplistic simulation to a FAT system (File Allocation Table.)

The allocation system devides the SRAM into blocks (512bytes) and manage the allocation and the freeing of spaces.

the project is implimented with Ceedling for unit testing.

### Note

This file system is not to be used in real applications, even though it works, it is intended for educational purposes to explore how file systems like the FAT works

### How to use

1. Mount the system
2. allocate a bock of bytes
3. Write and Read to the allocated space using the id.
4. free if needed.

You may look into the test folder for Test Asserts, and how to the routines are used.

### Ceedling

Simple ceedling commands

- `ceedling test:all` to test all modules
- `ceedling test:sram_alloc` to test the sram_alloc system