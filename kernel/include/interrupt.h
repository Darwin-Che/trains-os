#ifndef K_INTERRUPT_H
#define K_INTERRUPT_H

#include "debug_print.h"
#include "global_state.h"
#include "lib/include/timer.h"
#include "lib/include/rpi.h"

#ifdef DEBUG
#define INTERRUPT_QUANTUM (5 * 1000 * 1000) // us

#else
#define INTERRUPT_QUANTUM (10 * 1000) // us

#endif

#define GIC_INTR_ID_TIMER 97
#define GIC_INTR_ID_GPIO_0 145 // 96 - videocore base for gic 400, 49 - gic 0
#define GIC_INTR_ID_UART 153
#define GIC_INTR_ID_NONE 1023

#define INTR_ID_GIC(id) (id >> 16)
#define INTR_ID_SUB(id) (id & 0xFFFF)

#define INTR_ID_TIMER (GIC_INTR_ID_TIMER << 16)
#define INTR_ID_UART(id) ((GIC_INTR_ID_UART << 16) + (id))
#define INTR_ID_GPIO_0(id) ((GIC_INTR_ID_GPIO_0 << 16) + (id))

enum kUartIER {
  UART_IER_RHR = (0x1 << 0),
  UART_IER_THR = (0x1 << 1),
  UART_IER_MODEM = (0x1 << 3),
  UART_IER_CTS = (0x1 << 7),
};

void k_gic_enable();
void k_intr_handler();

// static inline void k_intr_uart_arm(enum UartChnlId chnl, enum kUartIER ier) {
//   int uart_ier_value = uart_read_register(0, chnl, UART_IER);
//   uart_write_register(0, chnl, UART_IER, uart_ier_value | ier);
// }

// static inline void k_intr_uart_unarm(enum UartChnlId chnl, enum kUartIER ier) {
//   int uart_ier_value = uart_read_register(0, chnl, UART_IER);
//   uart_write_register(0, chnl, UART_IER, uart_ier_value & ~(ier));
//   gpio->GPEDS0 |= (0x1 << 24);
// }

static inline void k_intr_timer_arm() {
  if (kg_gs->next_intr_time == 0) {
    // need to set the first interrupt
    time_intr_reset();
    kg_gs->next_intr_time = get_current_time_u64() + INTERRUPT_QUANTUM;
    time_intr_set(kg_gs->next_intr_time);
    return;
  }
  // Schedule the next interrupt
  kg_gs->next_intr_time = kg_gs->next_intr_time + INTERRUPT_QUANTUM;
  time_intr_set(kg_gs->next_intr_time);
}

static inline void k_intr_timer_unarm() { time_intr_reset(); }

static inline void k_intr_uart_arm(uint32_t uart_id) {
  uart_intr_arm(uart_id);
}

static inline void k_intr_uart_unarm(uint32_t uart_id) {
  uart_intr_unarm(uart_id);
}

#endif