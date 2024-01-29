#ifndef K_UAPI_H
#define K_UAPI_H

#include <stddef.h>
#include <stdint.h>

#define ke_nameserver_tid 2
#define NAME_CLOCK_SERVER "clock"
#define NAME_UART_MARKLIN_SERVER "marklin"
#define NAME_UART_TERM_SERVER "term"

#define KE_PRIO_MIN (20)
#define KE_PRIO_MAX (0)

#define KE_PRIO_UART_SERVER KE_PRIO_MAX + 2
#define KE_PRIO_UART_NOTIFIER KE_PRIO_MAX + 1

#define KE_INTR_ID_RANGE_MIN (16)

enum keIntrId
{
  KE_INTR_TIMER = KE_INTR_ID_RANGE_MIN,
  KE_INTR_TERM_INPUT,
  KE_INTR_TERM_OUTPUT,
  KE_INTR_MARKLIN_INPUT,
  KE_INTR_MARKLIN_OUTPUT,
  // This CTS wait event will unblock(return) if CTS has changed since
  // 1. last read of the MSR register
  // 2. return from this event
  KE_INTR_MARKLIN_CTS,
};

extern void ke_exit();
extern void ke_yield();
extern int ke_create(int priority, void (*function)());
extern int ke_my_tid();
extern int ke_my_parent_tid();

/*
>=0 the size of the message returned by the replying task. The actual reply is less than or equal to the size of the reply buffer provided for it. Longer replies are truncated.
-1  tid is not the task id of an existing task.
-2  send-receive-reply transaction could not be completed.
*/
extern int ke_send(int tid, const char *msg, size_t msglen, char *reply, size_t rplen);

/*
>=0	the size of the message sent by the sender (stored in tid). The actual message is less than or equal to the size of the message buffer supplied. Longer messages are truncated.
*/
extern int ke_recv(int *tid, char *msg, size_t msglen);

/*
>=0 the size of the reply message transmitted to the original sender task. If this is less than the size of the reply message, the message has been truncated.
-1  tid is not the task id of an existing task.
-2  tid is not the task id of a reply-blocked task.
*/
extern int ke_reply(int tid, const char *reply, size_t rplen);

/*
>= 0 for tid.
-1   if nameserver tid is wrong.
-2   if the name doesn't exist in the nameserver.
 */
extern int ke_who_is(const char *name);
extern int ke_register_as(const char *name);

/*
>=0 volatile data
-1  invalid event
-2 corrupted volatile data
*/
extern int ke_await_event(enum keIntrId event_type);

/*
>=0 time in ticks since the clock server initialized.
-1  tid is not a valid clock server task.
*/
extern int ke_time(int tid);

/*
>=0 success. The current time returned (as in ke_time()).
-1  tid is not a valid clock server task.
-2  negative delay.
*/
extern int ke_delay(int tid, int ticks);

/*
>=0 success. The current time returned (as in ke_time()).
-1  tid is not a valid clock server task.
-2  negative delay.
*/
extern int ke_delay_until(int tid, int tick);

/*
0  success
<0 error
*/
extern int ke_uart_write_reg(int channel, char reg, char data);

/*
>=0 success. return register value
<0  error
*/
extern int ke_uart_read_reg(int channel, char reg);

/*
>=0 new character from the given UART.
-1  tid is not a valid uart server task.
-2  uart server waitqueue is full.
*/
extern int ke_getc(int tid, int channel);

/*
0  success.
-1 tid is not a valid uart server task.
-2  uart server waitqueue is full.
*/
extern int ke_putc(int tid, int channel, char ch);

/*
0  success.
-1 tid is not a valid uart server task.
-2  uart server waitqueue is full.

Blocking for Marklin
Non-blocking for terminal
*/
extern int ke_puts(int tid, int channel, const char *str, int len);

/*
Reboot
*/
extern void ke_quit();

struct keSysHealth
{
  int idle_percent;
};

extern int ke_sys_health(struct keSysHealth *h);

/*
Request memory pages
mem_sz is just a number (internally we will cast it up to closest power of 2 and alloc that amount, alignment is also at that number)
When alloc failed, return NULL
*/
extern int ke_mmap(void **target, uint64_t mem_sz);

#endif
