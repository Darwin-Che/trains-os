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

extern void ke_exit();
extern void ke_yield();
extern int ke_create(int priority, const char * args, size_t args_len);
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
extern int ke_await_event(uint32_t irq);

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
Debug Print
*/
extern int ke_print_raw(const char * msg, int len);

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

/*
Set up a quarature encoder and return the id
*/
extern int ke_quadrature_encoder_init(uint32_t pin_a, uint32_t pin_b);

struct keQuadEncoderStat
{
  uint64_t forward_cnt;
  uint64_t backward_cnt;
  uint64_t invalid_1_cnt;
  uint64_t invalid_2_cnt;
  uint64_t debug;
};

static void ke_quad_encoder_stat_clear(struct keQuadEncoderStat* stat) {
  stat->forward_cnt = 0;
  stat->backward_cnt = 0;
  stat->invalid_1_cnt = 0;
  stat->invalid_2_cnt = 0;
  stat->debug = 0;
}

/*
Return the number of forward ticks (can be negative)
*/
extern int ke_quadrature_encoder_get(int id, struct keQuadEncoderStat * stat);

#endif
