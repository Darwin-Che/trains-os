#ifndef U_CLOCK_H
#define U_CLOCK_H

struct TickInterval
{
  int begin;
  int end;
};

void clock_server_main();
void clock_notifier_main();

void clock_delay_main();

#endif