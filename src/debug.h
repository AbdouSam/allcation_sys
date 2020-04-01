#ifndef DEBUG_H
#define DEBUG_H


#define event_dbg_msg(format, ...) dbg_msg(format, ##__VA_ARGS__)

#define dbg_msg(format, ...)     printf(format, ##__VA_ARGS__)

#endif /* DEBUG_H */