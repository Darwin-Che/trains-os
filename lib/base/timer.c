#include <stdint.h>
#include "timer.h"

void parse_time(struct PiTime *time)
{
  uint64_t cur_time = time->ts;
  time->mmsec = cur_time % 1000;
  cur_time /= 1000;
  time->msec = cur_time % 1000;
  cur_time /= 1000;
  time->sec = cur_time % 60;
  cur_time /= 60;
  time->min = cur_time % 60;
  cur_time /= 60;
  time->hr = cur_time;
}

// MM:SS.T
void format_current_time(struct PiTime *time)
{
  parse_time(time);
  char *str = time->str;
  uint16_t idx = 0;
  str[idx++] = time->min / 10 + '0';
  str[idx++] = time->min % 10 + '0';
  str[idx++] = ':';
  str[idx++] = time->sec / 10 + '0';
  str[idx++] = time->sec % 10 + '0';
  str[idx++] = '.';
  str[idx++] = time->msec / 100 + '0';
  str[idx++] = '\0';
}

void get_current_time(struct PiTime *time)
{
  time->ts = get_current_time_u64();
  parse_time(time);
  format_current_time(time);
}

bool time_equal_approx(struct PiTime *t1, struct PiTime *t2)
{
  return t1->sec == t2->sec && t1->msec / 100 == t2->msec / 100;
}

bool time_is_before(struct PiTime *t1, struct PiTime *t2)
{
  return t1->ts < t2->ts;
}

void time_add(struct PiTime *t, struct PiDuration *d)
{
  t->ts += d->msec * 1000 + d->mmsec;
  parse_time(t);
  format_current_time(t);
}

static struct PiDuration d;
struct PiDuration *duration_tmp(uint32_t msec)
{
  d.msec = msec;
  d.mmsec = 0;
  return &d;
}

struct PiDuration *time_diff(struct PiTime *t1, struct PiTime *t2)
{
  d.msec = (t1->ts - t2->ts) / 1000;
  d.mmsec = (t1->ts - t2->ts) % 1000;
  return &d;
}

struct PiDuration *duration_max(struct PiDuration *d1, struct PiDuration *d2)
{
  if (d1->msec == d2->msec && d1->mmsec > d2->mmsec)
    return d1;
  if (d1->msec > d2->msec)
    return d1;
  return d2;
}
