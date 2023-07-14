#include "kernel/uapi.h"
#include "lib/include/rpi.h"
#include "lib/include/ring_buffer.h"
#include "msg/msg.h"
#include "uart.h"
#include "uart_priv.h"

#define OUTPUT_QUEUE_CAP 64

// #define DEBUG 1

enum OutputStatus
{
  OS_READY,
  OS_WAIT_CTS,
  OS_WAIT_TX,
};

struct OutputWaitElem
{
  int tid;
  int progress;
  struct mUartPutsReq req;
};

struct UartServerMarklin
{
  int output_notifier_tid;
  int input_notifier_tid;
  int cts_notifier_tid;

  InputBufType input_buf;
  InputWaitQueueType input_wqueue;

  RB_TYPE(struct OutputWaitElem, OUTPUT_QUEUE_CAP)
  output_wqueue;

  enum OutputStatus output_status;
};

static void uart_server_init(struct UartServerMarklin *s)
{
  RB_INIT(&s->input_buf, INPUT_BUF_CAP);
  RB_INIT(&s->input_wqueue, INPUT_QUEUE_CAP);
  RB_INIT(&s->output_wqueue, OUTPUT_QUEUE_CAP);

  s->output_notifier_tid = ke_create(KE_PRIO_UART_NOTIFIER, uart_notifier_main);
  ASSERT_MSG(s->output_notifier_tid >= 0, "output_notifier_tid < 0");
  s->input_notifier_tid = ke_create(KE_PRIO_UART_NOTIFIER, uart_notifier_main);
  ASSERT_MSG(s->input_notifier_tid >= 0, "input_notifier_tid < 0");
  s->cts_notifier_tid = ke_create(KE_PRIO_UART_NOTIFIER, uart_notifier_main);
  ASSERT_MSG(s->cts_notifier_tid >= 0, "cts_notifier_tid < 0");

  s->output_status = OS_READY;
}

static void uart_server_init_notifiers(struct UartServerMarklin *s)
{
  MSG_INIT(sendmsg, mUartNotifierReq);
  MSGBOX_T(replybox, sizeof(struct mUartNotifierResp));

  sendmsg.intr_id = KE_INTR_MARKLIN_OUTPUT;
  MSG_SEND(s->output_notifier_tid, &sendmsg, &replybox);
  MSGBOX_CAST(&replybox, mUartNotifierResp, _1);

  sendmsg.intr_id = KE_INTR_MARKLIN_INPUT;
  MSG_SEND(s->input_notifier_tid, &sendmsg, &replybox);
  MSGBOX_CAST(&replybox, mUartNotifierResp, _2);

  sendmsg.intr_id = KE_INTR_MARKLIN_CTS;
  MSG_SEND(s->cts_notifier_tid, &sendmsg, &replybox);
  MSGBOX_CAST(&replybox, mUartNotifierResp, _3);
}

static void uart_flush_output(struct UartServerMarklin *s)
{
  // Because of CTS, can only flush one byte at a time
  struct OutputWaitElem *elem;

  // Only accept flushes when the status is ready
  if (s->output_status != OS_READY)
    return;

  RB_PEEK(&s->output_wqueue, elem);
  if (elem == NULL)
    return;

  if (ke_uart_read_reg(MARKLIN_CHANNEL, UART_TXLVL) == 0)
  {
    // Cannot deliver the byte, change to TX blocked
#ifdef DEBUG
    ke_print("OS_WAIT_TX\r\n");
#endif
    s->output_status = OS_WAIT_TX;
    MSG_INIT(reply_notifier, mUartEmptyResp);
    MSG_REPLY(s->output_notifier_tid, &reply_notifier);
    return;
  }

  // Be ready to receive CTS
  s->output_status = OS_WAIT_CTS;

  // Write the byte
#ifdef DEBUG
  char debug_msg[] = "000 Write Byte\r\n";
  debug_msg[0] = '0' + elem->req.str[elem->progress] / 100;
  debug_msg[1] = '0' + (elem->req.str[elem->progress] / 10) % 10;
  debug_msg[2] = '0' + elem->req.str[elem->progress] % 10;
  ke_print(debug_msg);
#endif
  ke_uart_write_reg(MARKLIN_CHANNEL, UART_THR, elem->req.str[elem->progress]);
  elem->progress++;
  if (elem->progress >= elem->req.str_len)
  {
    // This elem is finished
    if (elem->req.blocking)
    {
      MSG_INIT(reply_putc, mUartPutsResp);
      reply_putc.result = 0;
      MSG_REPLY(elem->tid, &reply_putc);
    }
    RB_POP(&s->output_wqueue);
  }

  uart_flush_output(s);
}

void uart_server_marklin_main()
{
  uart_drain_input(MARKLIN_CHANNEL);

  ke_register_as(NAME_UART_MARKLIN_SERVER);
  struct UartServerMarklin server;
  // ********* Initialization *********
  uart_server_init(&server);

  // ********* Give notifiers the initial values *********
  uart_server_init_notifiers(&server);

  // ********* Actual loop *********
  {
    MSGBOX_T(recvbox, 4096); // A large buffer
    MSG_INIT(reply_notifier, mUartEmptyResp);
    MSG_INIT(reply_getc, mUartGetcResp);
    MSG_INIT(reply_putc, mUartPutsResp);
    int tid;

    while (1)
    {
      MSG_RECV(&tid, &recvbox);
      if (MSGBOX_IS(&recvbox, mUartGetcReq))
      {
#ifdef DEBUG
        ke_print("mUartGetcReq Marklin\r\n");
#endif
        MSGBOX_CAST(&recvbox, mUartGetcReq, req);
        struct InputWaitElem *elem;
        RB_PUSH_PRE(&server.input_wqueue, elem);
        if (elem == NULL)
        {
          reply_getc.result = -2;
          MSG_REPLY(tid, &reply_getc);
          continue;
        }
        elem->tid = tid;
        ASSERT_MSG(req->channel == MARKLIN_CHANNEL, "Send terminal getc to marklin uart server!\r\n");
        uart_flush_input(&server.input_buf, &server.input_wqueue, MARKLIN_CHANNEL, server.input_notifier_tid);
      }
      else if (MSGBOX_IS(&recvbox, mUartPutsReq))
      {
#ifdef DEBUG
        ke_print("mUartPutsReq Marklin\r\n");
#endif
        MSGBOX_CAST(&recvbox, mUartPutsReq, req);

        ASSERT_MSG(req->channel == MARKLIN_CHANNEL, "Send terminal puts to marklin uart server!\r\n");

        if (req->str_len == 0)
        {
          reply_putc.result = 0;
          MSG_REPLY(tid, &reply_putc);
          continue;
        }

        struct OutputWaitElem *elem;
        RB_PUSH_PRE(&server.output_wqueue, elem);
        if (elem == NULL)
        {
          reply_putc.result = -2;
          MSG_REPLY(tid, &reply_putc);
          continue;
        }

        if (!req->blocking)
        {
          MSG_INIT(reply_putc, mUartPutsResp);
          reply_putc.result = 0;
          MSG_REPLY(tid, &reply_putc);
        }
        elem->tid = tid;
        elem->progress = 0;
        util_memcpy(&elem->req, req, sizeof(struct mUartPutsReq));

        uart_flush_output(&server);
      }
      else if (MSGBOX_IS(&recvbox, mUartMarklinInputReq))
      {
#ifdef DEBUG
        ke_print("mUartMarklinInputReq Marklin\r\n");
#endif
        uart_flush_input(&server.input_buf, &server.input_wqueue, MARKLIN_CHANNEL, server.input_notifier_tid);
      }
      else if (MSGBOX_IS(&recvbox, mUartMarklinOutputReq))
      {
#ifdef DEBUG
        ke_print("mUartMarklinOutputReq Marklin\r\n");
#endif
        if (server.output_status == OS_WAIT_TX)
        {
          server.output_status = OS_READY;
          uart_flush_output(&server);
        }
        else if (server.output_status == OS_READY)
        {
          static bool first = true;
          if (!first)
          {
            printf("Marklin Uart Server received unexpected TX (1)\r\n");
            assert_fail();
          }
          first = false;
        }
        else if (server.output_status == OS_WAIT_CTS)
        {
          printf("Marklin Uart Server received unexpected TX (1)\r\n");
          assert_fail();
        }
      }
      else if (MSGBOX_IS(&recvbox, mUartMarklinCtsReq))
      {
#ifdef DEBUG
        ke_print("mUartMarklinCtsReq Marklin\r\n");
#endif
        // Always wait for CTS
        MSG_INIT(reply_notifier, mUartEmptyResp);
        MSG_REPLY(server.cts_notifier_tid, &reply_notifier);

        if (server.output_status == OS_WAIT_CTS)
        {
          if (ke_uart_read_reg(MARKLIN_CHANNEL, UART_TXLVL) != 0)
          {
            server.output_status = OS_READY;
            uart_flush_output(&server);
          }
          else
          {
#ifdef DEBUG
            ke_print("OS_WAIT_TX (2)\r\n");
#endif
            server.output_status = OS_WAIT_TX;
            // Start to wait for output buffer
            MSG_INIT(reply_notifier, mUartEmptyResp);
            MSG_REPLY(server.output_notifier_tid, &reply_notifier);
          }
        }
        // else if (server.output_status == OS_READY)
        // {
        //   static bool first = true;
        //   if (!first)
        //   {
        //     printf("Marklin Uart Server received unexpected CTS (1)\r\n");
        //     assert_fail();
        //   }
        //   first = false;
        // }
        // else if (server.output_status == OS_WAIT_TX)
        // {
        //   printf("Marklin Uart Server received unexpected CTS (2)\r\n");
        //   assert_fail();
        // }
      }
      else
      {
        printf("Marklin Uart Server received unrecognized request\r\n");
        assert_fail();
      }
    }
  }
}
