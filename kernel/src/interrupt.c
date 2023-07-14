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

enum kUartIntrCode
{
  // SC 16 - Page 24, Table 14 - bits 5 - 1
  UART_IIR_RECV_LINE_STATUS = 0x3,
  UART_IIR_RECV_TIMEOUT = 0x6,
  UART_IIR_RHR = 0x2,
  UART_IIR_THR = 0x1,
  UART_IIR_MODEM = 0x0,
  UART_IIR_INPUT_PIN_STATE_CHANGE = 0x18,
  UART_IIR_XOFF_SIGNAL = 0x8,
  UART_IIR_CTS_RTS_CHANGE = 0x10,
};

void k_gic_enable()
{
  // GIC_INTR_ID_TIMER = 3 * 32 + 1
  pi_gicd->GICD_ISENABLERn[(GIC_INTR_ID_TIMER / 32)] |= (0x1 << (GIC_INTR_ID_TIMER % 32));
  // GIC_INTR_GPIO_0 (bank 0) = 4 * 32 + 17
  pi_gicd->GICD_ISENABLERn[(GIC_INTR_ID_GPIO_0 / 32)] |= (0x1 << (GIC_INTR_ID_GPIO_0 % 32));
  // target interrupt to core 0
  pi_gicd->GICD_ITARGETSRn[GIC_INTR_ID_TIMER] |= (0x1 << 0);
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

static inline int k_intr_wakeup_tasks(enum keIntrId intr_id)
{
  struct kTaskDsp *td, *tmp;
  uint8_t number_of_events_triggered = 0;

  DL_FOREACH_SAFE2(kg_gs->scheduler.event_blocked, td, tmp, next_sched)
  {
    if ((enum keIntrId)td->state == intr_id)
    {
      k_sched_remove_event_wait(&kg_gs->scheduler, td);
      k_sched_add_ready(&kg_gs->scheduler, td);
      number_of_events_triggered += 1;
    }
  }

  if (intr_id == KE_INTR_MARKLIN_CTS)
    ASSERT_MSG(number_of_events_triggered <= 1,
               "Unexpected number of events triggered: %llu for eventid: %llu",
               number_of_events_triggered, intr_id);
  else
    ASSERT_MSG(number_of_events_triggered == 1,
               "Unexpected number of events triggered: %llu for eventid: %llu",
               number_of_events_triggered, intr_id);
  return number_of_events_triggered;
}

static inline void process_intr_timer()
{
  DEBUG_PRINT("Kernel Process Timer Interrupt\r\n");

  // Turn off interrupt
  k_intr_timer_unarm();

  // Wake up the clock notifier
  k_intr_wakeup_tasks(KE_INTR_TIMER);
}

static inline bool process_iir_val(enum UartChnlId channel, int iir_val)
{
  DEBUG_PRINT("Channel: %d\r\n", channel);

  if ((iir_val & 0x1) != 0)
  {
    DEBUG_PRINT("No interrupt pending, val: %d\r\n", iir_val);
    return false;
  }

  int iir_code = ((iir_val >> 1) & 0x1F); // get bits 5:1

  switch (iir_code)
  {
  case UART_IIR_RECV_TIMEOUT:
    DEBUG_PRINT("Receive timeout status interrupt \r\n");
    // fall through
  case UART_IIR_RHR:
    DEBUG_PRINT("RHR interrupt \r\n");
    {
      k_intr_uart_unarm(channel, UART_IER_RHR);

      if (channel == TERM_CHANNEL)
        k_intr_wakeup_tasks(KE_INTR_TERM_INPUT);
      else if (channel == MARKLIN_CHANNEL)
        k_intr_wakeup_tasks(KE_INTR_MARKLIN_INPUT);
      else
        ASSERT_MSG(false, "Invalid Param: channel id %d\r\n", channel);

      return true;
    }

  case UART_IIR_THR:
    DEBUG_PRINT("THR interrupt \r\n");
    {
      k_intr_uart_unarm(channel, UART_IER_THR);

      if (channel == TERM_CHANNEL)
        k_intr_wakeup_tasks(KE_INTR_TERM_OUTPUT);
      else if (channel == MARKLIN_CHANNEL)
        k_intr_wakeup_tasks(KE_INTR_MARKLIN_OUTPUT);
      else
        ASSERT_MSG(false, "Invalid Param: channel id %d\r\n", channel);

      return true;
    }

  case UART_IIR_CTS_RTS_CHANGE:
    DEBUG_PRINT("CTS/RTS state change\r\n");
    {
      k_intr_uart_unarm(channel, UART_IER_CTS);

      if (channel == TERM_CHANNEL)
      {
        ASSERT_MSG(false, "Receive CTS on Term Channel!\r\n");
      }
      else if (channel == MARKLIN_CHANNEL)
        k_intr_wakeup_tasks(KE_INTR_MARKLIN_CTS);
      else
        ASSERT_MSG(false, "Invalid Param: channel id %d\r\n", channel);

      return true;
    }

  case UART_IIR_MODEM:
    DEBUG_PRINT("Modem interrupt \r\n");
    {
      if (channel == TERM_CHANNEL)
      {
        ASSERT_MSG(false, "Receive Modem on Term Channel!\r\n");
      }
      else if (channel == MARKLIN_CHANNEL)
      {
        char msr = uart_read_register(0, MARKLIN_CHANNEL, UART_MSR);
        DEBUG_PRINT("Modem On Marklin! %X\r\n", msr);
        if (msr & 0x1)
          k_intr_wakeup_tasks(KE_INTR_MARKLIN_CTS);
        k_intr_uart_unarm(channel, UART_IER_MODEM);
      }
      else
        ASSERT_MSG(false, "Invalid Param: channel id %d\r\n", channel);
      return true;
    }

    /* **************** BELOW ARE DEFENSIVE BRANCHES **************** */

  case UART_IIR_RECV_LINE_STATUS:
    DEBUG_PRINT("Receive line status interrupt \r\n");
    assert_fail();
    break;
  case UART_IIR_INPUT_PIN_STATE_CHANGE:
    DEBUG_PRINT("Input pin state change \r\n");
    assert_fail();
    break;
  case UART_IIR_XOFF_SIGNAL:
    DEBUG_PRINT("Xoff Signal \r\n");
    assert_fail();
    break;
  default:
    DEBUG_PRINT("Unknown iir value: %d", iir_val);
    printf("Unknown iir value: %d", iir_val);
    assert_fail();
    break;
  }
  return false;
}

static inline void process_intr_uart(char iir_term, char iir_marklin)
{
  DEBUG_PRINT("Kernel Process Uart Interrupt\r\n");
  bool processed = false;

  // Check if interrupt happened in terminal
  processed = process_iir_val(TERM_CHANNEL, iir_term) || processed;

  // Check if interrupt happened in marklin
  processed = process_iir_val(MARKLIN_CHANNEL, iir_marklin) || processed;

  if (!processed)
  {
    DEBUG_PRINT("SPURIOUS UART Interrupt!!\r\n");
    // assert_fail();
  }
}

void k_intr_handler()
{
  while (1)
  {
    uint32_t id = k_gic_iar();
    if (id == GIC_INTR_ID_TIMER)
    {
      DEBUG_PRINT("Got interrupt from TIMER!! \r\n");
      // Process the timer & turn off interrupt
      process_intr_timer();
    }
    else if (id == GIC_INTR_ID_GPIO_0)
    {
      // Must read IIR before debug prints
      char iir_term = uart_read_register(0, TERM_CHANNEL, UART_IIR);
      char iir_marklin = uart_read_register(0, MARKLIN_CHANNEL, UART_IIR);

      DEBUG_PRINT("Got interrupt from UART!! \r\n");
      // Process & turn off interrupt
      process_intr_uart(iir_term, iir_marklin);
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