#ifndef TIMER_H
#define TIMER_H

#include "util.h"

#define CUR_TIME_STR_SZ 8
struct PiTime
{
  uint64_t ts;
  char str[CUR_TIME_STR_SZ];
  uint16_t mmsec;
  uint16_t msec;
  uint16_t sec;
  uint16_t min;
  uint16_t hr;
};

struct PiDuration
{
  uint16_t msec;
  uint16_t mmsec;
};

struct PiTimer
{
  uint32_t control_status;
  uint32_t clock_lo;
  uint32_t clock_hi;
  uint32_t cmp_0;
  uint32_t cmp_1;
  uint32_t cmp_2;
  uint32_t cmp_3;
};

static volatile struct PiTimer *const pi_timer = (struct PiTimer *)0xFE003000;

static inline uint64_t get_current_time_u64()
{
  uint64_t cur_time = pi_timer->clock_lo;
  cur_time |= ((uint64_t)pi_timer->clock_hi) << 32;
  return cur_time;
}

void get_current_time(struct PiTime *time);

void format_current_time(struct PiTime *time);

bool time_equal_approx(struct PiTime *t1, struct PiTime *t2);

bool time_is_before(struct PiTime *t1, struct PiTime *t2);

void time_add(struct PiTime *t, struct PiDuration *d);

struct PiDuration *duration_tmp(uint32_t msec);

struct PiDuration *time_diff(struct PiTime *t1, struct PiTime *t2);

struct PiDuration *duration_max(struct PiDuration *d1, struct PiDuration *d2);

// static inline void time_intr_after(uint64_t msec)
// {
//   pi_timer->cmp_1 = pi_timer->clock_lo + msec * 1000;
// }

static inline void time_intr_set(uint64_t ts)
{
  pi_timer->cmp_1 = ts;
}

static inline void time_intr_reset()
{
  pi_timer->control_status |= (0x1 << 1);
  pi_timer->cmp_1 = 0;
}

#endif