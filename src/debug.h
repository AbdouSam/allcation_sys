/**
  * @file  : debug.h
  * @brief : debug messages, printf in case of test
  */
#ifndef DEBUG_H
#define DEBUG_H


#define dbg_msg(format, ...)     printf(format, ##__VA_ARGS__)

#endif /* DEBUG_H */