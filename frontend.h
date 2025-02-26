#ifndef FRONTEND_H
#define FRONTEND_H

#define FETCH_WIDTH 4
#define COMMIT_WIDTH 4
#define RESET_PC 0x1008c
#define PMEM_OFFSET RESET_PC

/*#define IO_version*/

#define DEBUG_PRINT 0
#define DEBUG_LOG(fmt, ...)                                                    \
  do {                                                                         \
    if (DEBUG_PRINT)                                                           \
      printf(fmt, ##__VA_ARGS__);                                              \
  } while (0)

/*#define IO_GEN_MODE*/
extern int io_gen_cnt;
#endif
