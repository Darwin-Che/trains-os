#include <stdint.h>
#include "lib/include/printf.h"
#include "lib/include/assert.h"
#include "lib/include/timer.h"
#include "task.h"
#include "scheduler.h"
#include "global_state.h"
#include "lib/include/utlist.h"
#include "debug_print.h"
#include "interrupt.h"
#include "assert.h"
#include "lib/include/rpi.h"

// explicitly setting GICD typer with values might lead to faster execution
struct PiGicD
{
  uint32_t GICD_CTLR;             // 0x0 - 0x4
  char pad1[0x100 - 0x4];         // 0x4 - 0x100
  uint32_t GICD_ISENABLERn[0x10]; // 0x100 - 0x140
  char pad3[0x6C0];               // 0x140 - 0x800
  uint8_t GICD_ITARGETSRn[0x3FC]; // 0x800 - 0xBFC
};

struct PiGicC
{
  uint32_t GICC_CTLR;
  uint32_t _GICC_PMR;
  uint32_t _GICC_BPR;
  uint32_t GICC_IAR;
  uint32_t GICC_EOIR;
};

#define PI_GIC_BASE 0xff840000

volatile struct PiGicD *pi_gicd = (struct PiGicD *)(PI_GIC_BASE + 0x1000);
volatile struct PiGicC *pi_gicc = (struct PiGicC *)(PI_GIC_BASE + 0x2000);

void k_gic_enable()
{
  // GIC_INTR_ID_TIMER = 3 * 32 + 1
  pi_gicd->GICD_ISENABLERn[(GIC_INTR_ID_TIMER / 32)] |= (0x1 << (GIC_INTR_ID_TIMER % 32));
  // GIC_INTR_UART = 4 * 32 + 25 = 153
  pi_gicd->GICD_ISENABLERn[(GIC_INTR_ID_UART / 32)] |= (0x1 << (GIC_INTR_ID_UART % 32));
  // GIC_INTR_GPIO_0 (bank 0) = 4 * 32 + 17 = 145
  pi_gicd->GICD_ISENABLERn[(GIC_INTR_ID_GPIO_0 / 32)] |= (0x1 << (GIC_INTR_ID_GPIO_0 % 32));
  // target interrupt to core 0
  pi_gicd->GICD_ITARGETSRn[GIC_INTR_ID_TIMER] |= (0x1 << 0);
  pi_gicd->GICD_ITARGETSRn[GIC_INTR_ID_UART] |= (0x1 << 0);
  pi_gicd->GICD_ITARGETSRn[GIC_INTR_ID_GPIO_0] |= (0x1 << 0);

  pi_gicd->GICD_CTLR = 0x1; // only enables group 0 interrupts but not group 1 interrupts
  pi_gicc->GICC_CTLR = 0x1; // forwards group 1 interrupts - i think does nothing
}

static inline uint32_t k_gic_iar()
{
  return pi_gicc->GICC_IAR;
}

static inline void k_gic_eoir(uint32_t id)
{
  pi_gicc->GICC_EOIR = id;
}

static inline int k_intr_wakeup_tasks(uint32_t intr_id, uint32_t retval)
{
  struct kTaskDsp *td, *tmp;
  uint8_t number_of_events_triggered = 0;

  DL_FOREACH_SAFE2(kg_gs->scheduler.event_blocked, td, tmp, next_sched)
  {
    if (td->state == intr_id)
    {
      k_sched_remove_event_wait(&kg_gs->scheduler, td);
      td->syscall_retval = retval;
      k_sched_add_ready(&kg_gs->scheduler, td);
      number_of_events_triggered += 1;
    }
  }

  // if (intr_id == KE_INTR_MARKLIN_CTS)
  //   ASSERT_MSG(number_of_events_triggered <= 1,
  //              "Unexpected number of events triggered: %llu for eventid: %llu",
  //              number_of_events_triggered, intr_id);
  // else
  //   ASSERT_MSG(number_of_events_triggered == 1,
  //              "Unexpected number of events triggered: %llu for eventid: %llu",
  //              number_of_events_triggered, intr_id);
  return number_of_events_triggered;
}

static inline void process_intr_timer()
{
  DEBUG_PRINT("Kernel Process Timer Interrupt\r\n");

  // Turn off interrupt
  k_intr_timer_unarm();

  // Wake up the clock notifier
  k_intr_wakeup_tasks(INTR_ID_TIMER, 0);
}

static inline void process_intr_uart_id(uint32_t uart_id)
{
  uint32_t status = uart_intr_status(uart_id);
  if (status != 0) {
    // turnoff interrupt
    k_intr_uart_unarm(uart_id);

    // wakeup tasks
    k_intr_wakeup_tasks(INTR_ID_UART(uart_id), status);
  }
}

static inline void process_intr_uart()
{
  process_intr_uart_id(0);
  process_intr_uart_id(2);
  process_intr_uart_id(3);
  process_intr_uart_id(4);
  process_intr_uart_id(5);
}

static inline void process_intr_gpio_pin(int pin) {
  // Check for encoder
  struct kQuadEncoder * encoder = get_encoder_by_pin(&kg_gs->encoder_mgr, pin);
  if (encoder == NULL) {
    return;
  }

  // Read pins
  uint64_t pin_lvl = gpio_read();
  bool a_state = pin_lvl & (1 << encoder->pin_a);
  bool b_state = pin_lvl & (1 << encoder->pin_b);

  // printf("UDPATE %llu %d %d\r\n", get_current_time_u64(), a_state, b_state);
  encoder_update(encoder, a_state, b_state);

  // gpio_set_edge_trigger(encoder->pin_a, !a_state, a_state);
  // gpio_set_edge_trigger(encoder->pin_b, !b_state, b_state);
}

static inline void process_intr_gpio_bank(int bank_id) {
  static uint32_t gpio_banks[4] = {0, 28, 46, 58};

  uint32_t start = gpio_banks[bank_id];
  uint32_t end = gpio_banks[bank_id+1];

  uint64_t triggered = gpio_get_triggered();

  for (uint32_t pin = start; pin < end; pin += 1) {
    // Check if pin is triggered
    if (triggered & (1 << pin)) {
      process_intr_gpio_pin(pin);
    }
  }

  gpio_clear_triggered(start, end);
}

void k_intr_handler()
{
  DEBUG_PRINT("k_intr_handler!!\r\n");
  while (1)
  {
    uint32_t id = k_gic_iar();
    if (id == GIC_INTR_ID_TIMER)
    {
      DEBUG_PRINT("Got interrupt from TIMER!! \r\n");
      // Process the timer & turn off interrupt
      process_intr_timer();
    }
    else if (id == GIC_INTR_ID_UART)
    {
      process_intr_uart();
    }
    else if (id == GIC_INTR_ID_GPIO_0)
    {
      process_intr_gpio_bank(0);
    }
    else if (id == GIC_INTR_ID_NONE)
    {
      DEBUG_PRINT("Finish Kernel Interrupt Handling Loop\r\n");
      return;
    }
    else
    {
      DEBUG_PRINT("Got interrupt from unrecognized source, id: %llu \r\n", id);
      assert_fail();
    }
    k_gic_eoir(id);
  }
}