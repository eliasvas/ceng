#ifndef TIME_UTIL_H
#define TIME_UTIL_H
#include "base/base_inc.h"
#include <time.h>

static inline u64 get_nano_freq(void) {
  return 1000000000ULL;
}

static inline u64 get_time_ns(void) {
  struct timespec ts;
  //timespec_get(&ts, TIME_UTC);
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * get_nano_freq() + ts.tv_nsec;
} 

#endif
