#ifndef U_UART_PRIV_H
#define U_UART_PRIV_H

#include "common/ring_buffer.h"
#include "common/rpi.h"
#include "kernel/uapi.h"
#include "msg/msg.h"

#define INPUT_QUEUE_CAP 64
#define INPUT_BUF_CAP 256

struct InputWaitElem
{
  int tid;
};

typedef RB_TYPE(char, INPUT_BUF_CAP) InputBufType;
typedef RB_TYPE(struct InputWaitElem, INPUT_QUEUE_CAP) InputWaitQueueType;

static inline void uart_drain_input(enum UartChnlId chnl)
{
  while (ke_uart_read_reg(chnl, UART_RXLVL) != 0)
    ke_uart_read_reg(chnl, UART_RHR);
}

static inline void uart_flush_input(
    InputBufType *input_buf,
    InputWaitQueueType *input_wqueue,
    enum UartChnlId chnl,
    int input_notifier_tid)
{
  char *cptr;
  struct InputWaitElem *elem;

  while (1)
  {
    // Get all of the FIFO buffer
    while (ke_uart_read_reg(chnl, UART_RXLVL) != 0)
    {
      RB_PUSH_PRE(input_buf, cptr);
      if (cptr == NULL)
        break;
      *cptr = ke_uart_read_reg(chnl, UART_RHR);
    }

    while (1)
    {
      // Reply to some getc request
      RB_PEEK(input_wqueue, elem);
      // If there are no getc request left
      if (elem == NULL)
        return;

      RB_PEEK(input_buf, cptr);
      // But there is no characters received yet
      if (cptr == NULL)
        break;

      // We have received the byte required for this request
      MSG_INIT(reply_getc, mUartGetcResp);
      reply_getc.result = 0;
      reply_getc.ch = *cptr;
      MSG_REPLY(elem->tid, &reply_getc);

      RB_POP(input_buf);
      RB_POP(input_wqueue);
    }

    if (ke_uart_read_reg(TERM_CHANNEL, UART_RXLVL) == 0)
      break;
  }
  // Here we are out of input, arm the interrupt
  MSG_INIT(reply_notifier, mUartEmptyResp);
  MSG_REPLY(input_notifier_tid, &reply_notifier);
}

#endif
