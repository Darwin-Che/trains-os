#include "kernel/uapi.h"
#include "lib/include/rpi.h"
#include "lib/include/ring_buffer.h"
#include "msg/msg.h"
#include "uart.h"
#include "uart_priv.h"

#define OUTPUT_BUF_CAP 4096

// #define DEBUG 1

struct UartServerTerm
{
  int output_notifier_tid;
  int input_notifier_tid;

  InputBufType input_buf;
  InputWaitQueueType input_wqueue;

  RB_TYPE(char, OUTPUT_BUF_CAP)
  output_buf;
};

static void uart_server_init(struct UartServerTerm *s)
{
  RB_INIT(&s->input_buf, INPUT_BUF_CAP);
  RB_INIT(&s->input_wqueue, INPUT_QUEUE_CAP);
  RB_INIT(&s->output_buf, OUTPUT_BUF_CAP);

  s->output_notifier_tid = ke_create(KE_PRIO_UART_NOTIFIER, uart_notifier_main);
  ASSERT_MSG(s->output_notifier_tid >= 0, "output_notifier_tid >= 0");
  s->input_notifier_tid = ke_create(KE_PRIO_UART_NOTIFIER, uart_notifier_main);
  ASSERT_MSG(s->input_notifier_tid >= 0, "input_notifier_tid >= 0");
}

static void uart_server_init_notifiers(struct UartServerTerm *s, enum keIntrId output, enum keIntrId input)
{
  MSG_INIT(sendmsg, mUartNotifierReq);
  MSGBOX_T(replybox, sizeof(struct mUartNotifierResp));

  sendmsg.intr_id = output;
  MSG_SEND(s->output_notifier_tid, &sendmsg, &replybox);
  MSGBOX_CAST(&replybox, mUartNotifierResp, _1);

  sendmsg.intr_id = input;
  MSG_SEND(s->input_notifier_tid, &sendmsg, &replybox);
  MSGBOX_CAST(&replybox, mUartNotifierResp, _2);
}

static void uart_flush_output(struct UartServerTerm *s)
{
  char *c;

  RB_PEEK(&s->output_buf, c);
  if (c == NULL)
    return;

  while (ke_uart_read_reg(TERM_CHANNEL, UART_TXLVL) != 0)
  {
    ke_uart_write_reg(TERM_CHANNEL, UART_THR, *c);
    RB_POP(&s->output_buf);
    RB_PEEK(&s->output_buf, c);
    if (c == NULL)
      return;
  }

  // Here we are still left with output, arm the interrupt
  MSG_INIT(reply_notifier, mUartEmptyResp);
  MSG_REPLY(s->output_notifier_tid, &reply_notifier);
}

void uart_server_term_main()
{
  uart_drain_input(TERM_CHANNEL);

  ke_register_as(NAME_UART_TERM_SERVER);
  struct UartServerTerm server;
  // ********* Initialization *********
  uart_server_init(&server);

  // ********* Give notifiers the initial values *********
  uart_server_init_notifiers(&server, KE_INTR_TERM_OUTPUT, KE_INTR_TERM_INPUT);

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
        ke_print("mUartGetcReq\r\n");
#endif
        MSGBOX_CAST(&recvbox, mUartGetcReq, req);

        ASSERT_MSG(req->channel == TERM_CHANNEL, "Send marklin getc to term uart server!\r\n");

        struct InputWaitElem *elem;
        RB_PUSH_PRE(&server.input_wqueue, elem);
        if (elem == NULL)
        {
          reply_getc.result = -2;
          MSG_REPLY(tid, &reply_getc);
          continue;
        }
        elem->tid = tid;
        uart_flush_input(&server.input_buf, &server.input_wqueue, TERM_CHANNEL, server.input_notifier_tid);
      }
      else if (MSGBOX_IS(&recvbox, mUartPutsReq))
      {
#ifdef DEBUG
        ke_print("mUartPutsReq\r\n");
#endif
        MSGBOX_CAST(&recvbox, mUartPutsReq, req);

        ASSERT_MSG(req->channel == TERM_CHANNEL, "Send marklin putc to term uart server!\r\n");

        // check if there are enough room
        int left_room = OUTPUT_BUF_CAP - RB_LEN(&server.output_buf);
        if (left_room < req->str_len)
        {
          reply_putc.result = -2;
          MSG_REPLY(tid, &reply_putc);
          continue;
        }

        // Copy the string over
        char *c;
        for (int i = 0; i < req->str_len; i += 1)
        {
          RB_PUSH_PRE(&server.output_buf, c);
          ASSERT_MSG(c != NULL, "Defensive Branch\r\n");
          *c = req->str[i];
        }

        reply_putc.result = req->str_len;
        MSG_REPLY(tid, &reply_putc);

        uart_flush_output(&server);
      }
      else if (MSGBOX_IS(&recvbox, mUartTermInputReq))
      {
#ifdef DEBUG
        ke_print("mUartTermInputReq\r\n");
#endif
        uart_flush_input(&server.input_buf, &server.input_wqueue, TERM_CHANNEL, server.input_notifier_tid);
      }
      else if (MSGBOX_IS(&recvbox, mUartTermOutputReq))
      {
#ifdef DEBUG
        ke_print("mUartTermOutputReq\r\n");
#endif
        uart_flush_output(&server);
      }
    }
  }
}
