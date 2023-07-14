#include "lib/include/rpi.h"
#include "kernel/uapi.h"
#include "msg/msg.h"
#include "uart.h"

// TODO: Create a macros for the similar procedures fo the variations

void uart_notifier_main()
{
  int server_tid = ke_my_parent_tid();
#if WAIT_FOR_CTS == 2
  int clock_tid = ke_who_is(NAME_CLOCK_SERVER);
#endif
  enum keIntrId intr_id;

  // Receive parameters from parent
  {
    int tid;
    MSG_INIT(reply, mUartNotifierResp);
    MSGBOX_T(recvbox, sizeof(struct mUartNotifierReq));

    MSG_RECV(&tid, &recvbox);
    MSGBOX_CAST(&recvbox, mUartNotifierReq, notifer_type);
    MSG_REPLY(tid, &reply);

    intr_id = notifer_type->intr_id;
  }

  // Actual Work
  MSGBOX_T(replybox, sizeof(struct mUartEmptyResp));

  if (intr_id == KE_INTR_MARKLIN_INPUT)
  {
    MSG_INIT(msg, mUartMarklinInputReq);

    while (1)
    {
      MSG_SEND(server_tid, &msg, &replybox);
      MSGBOX_CAST(&replybox, mUartEmptyResp, _);
      ke_await_event(KE_INTR_MARKLIN_INPUT);
    }
  }
  else if (intr_id == KE_INTR_MARKLIN_CTS)
  {
    MSG_INIT(msg, mUartMarklinCtsReq);

    ke_uart_read_reg(MARKLIN_CHANNEL, UART_MSR); // Clear at the program's start
    while (1)
    {
      MSG_SEND(server_tid, &msg, &replybox);
      MSGBOX_CAST(&replybox, mUartEmptyResp, _);
#if WAIT_FOR_CTS == 2
      ke_delay(clock_tid, 1);
#endif
#if WAIT_FOR_CTS == 1 // Track b
      // ke_print(ke_who_is(NAME_DSB_DEBUG), "CTS Wait\r\n");
      while (1)
      {
        ke_await_event(KE_INTR_MARKLIN_CTS);
        char msr = ke_uart_read_reg(MARKLIN_CHANNEL, UART_MSR);
        if ((msr & (0x1 << 4)) == 0)
          break;
      }
      // ke_print(ke_who_is(NAME_DSB_DEBUG), "CTS down\r\n");
      while (1)
      {
        ke_await_event(KE_INTR_MARKLIN_CTS);
        char msr = ke_uart_read_reg(MARKLIN_CHANNEL, UART_MSR);
        if (msr & (0x1 << 4))
          break;
      }
      // ke_print(ke_who_is(NAME_DSB_DEBUG), "CTS up\r\n");
#elif WAIT_FOR_CTS == 0 // Track A
      // ke_print(ke_who_is(NAME_DSB_DEBUG), "CTS Wait\r\n");
      while (1)
      {
        ke_await_event(KE_INTR_MARKLIN_CTS);
        char msr = ke_uart_read_reg(MARKLIN_CHANNEL, UART_MSR);
        if ((msr & (0x1 << 4)))
          break;
      }
      // ke_print(ke_who_is(NAME_DSB_DEBUG), "CTS down\r\n");
      while (1)
      {
        ke_await_event(KE_INTR_MARKLIN_CTS);
        char msr = ke_uart_read_reg(MARKLIN_CHANNEL, UART_MSR);
        if ((msr & (0x1 << 4)) == 0)
          break;
      }
      // ke_print(ke_who_is(NAME_DSB_DEBUG), "CTS up\r\n");
#endif
    }
  }
  else if (intr_id == KE_INTR_MARKLIN_OUTPUT)
  {
    MSG_INIT(msg, mUartMarklinOutputReq);

    while (1)
    {
      MSG_SEND(server_tid, &msg, &replybox);
      MSGBOX_CAST(&replybox, mUartEmptyResp, _);
      ke_await_event(KE_INTR_MARKLIN_OUTPUT);
    }
  }
  else if (intr_id == KE_INTR_TERM_INPUT)
  {
    MSG_INIT(msg, mUartTermInputReq);

    while (1)
    {
      MSG_SEND(server_tid, &msg, &replybox);
      MSGBOX_CAST(&replybox, mUartEmptyResp, _);
      ke_await_event(KE_INTR_TERM_INPUT);
    }
  }
  else if (intr_id == KE_INTR_TERM_OUTPUT)
  {
    MSG_INIT(msg, mUartTermOutputReq);

    while (1)
    {
      MSG_SEND(server_tid, &msg, &replybox);
      MSGBOX_CAST(&replybox, mUartEmptyResp, _);
      ke_await_event(KE_INTR_TERM_OUTPUT);
    }
  }
}