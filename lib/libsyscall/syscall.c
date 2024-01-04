#include "uapi.h"
#include "../../msg/msg.h"
#include "lib/include/rpi.h"

#define KE_WHO_IS_RETRY_LIMIT 50
#define KE_WHO_IS_RETRY_SEP 5 // ticks

int ke_who_is(const char *name)
{
  MSG_INIT(req, mNsWhoisReq);
  util_strncpy(req.name, sizeof(req.name), name);
  MSGBOX_T(resp, sizeof(struct mNsWhoisResp));

  int clock_tid = -1;

  for (int i = 0; i < KE_WHO_IS_RETRY_LIMIT; i += 1)
  {
    MSG_SEND(ke_nameserver_tid, &req, &resp);

    if (resp.len == -1)
    {
      ASSERT_MSG(false, "Invalid name server id");
      return -1;
    }

    MSGBOX_CAST(&resp, mNsWhoisResp, resp_unwrapped);
    if (resp_unwrapped->tid >= 0)
      return resp_unwrapped->tid;

    if (clock_tid == -1)
      clock_tid = ke_who_is(NAME_CLOCK_SERVER);

    ke_delay(clock_tid, KE_WHO_IS_RETRY_SEP);
  }

  ASSERT_MSG(false, "No registration found for name: %s", req.name);
  return -2;
}

int ke_register_as(const char *name)
{
  MSG_INIT(req, mNsRegisterReq);
  util_strncpy(req.name, sizeof(req.name), name);
  MSGBOX_T(resp, sizeof(struct mEmptyMsg));

  MSG_SEND(ke_nameserver_tid, &req, &resp);

  if (resp.len == -1)
    return -1;

  MSGBOX_CAST(&resp, mEmptyMsg, _);

  return 0;
}

int ke_time(int tid)
{
  MSG_INIT(req, mCkTimeReq);
  MSGBOX_T(replybox, sizeof(struct mCkResp));

  MSG_SEND(tid, &req, &replybox);

  if (replybox.len == -1)
    return -1;

  MSGBOX_CAST(&replybox, mCkResp, reply);

  return reply->tick;
}

int ke_delay(int tid, int ticks)
{
  MSG_INIT(req, mCkDelayReq);
  req.ticks = ticks;
  MSGBOX_T(replybox, sizeof(struct mCkResp));

  MSG_SEND(tid, &req, &replybox);

  if (replybox.len == -1)
    return -1;

  MSGBOX_CAST(&replybox, mCkResp, reply);

  return reply->tick; // the clock server return -2 for negative delay
}

int ke_delay_until(int tid, int tick)
{
  MSG_INIT(req, mCkDelayUntilReq);
  req.tick = tick;
  MSGBOX_T(replybox, sizeof(struct mCkResp));

  MSG_SEND(tid, &req, &replybox);

  if (replybox.len == -1)
    return -1;

  MSGBOX_CAST(&replybox, mCkResp, reply);

  return reply->tick; // the clock server return -2 for negative delay
}

int ke_getc(int tid, int channel)
{
  MSG_INIT(req, mUartGetcReq);
  req.channel = channel;
  MSGBOX_T(replybox, sizeof(struct mUartGetcResp));

  MSG_SEND(tid, &req, &replybox);

  if (replybox.len == -1)
    return -1;

  MSGBOX_CAST(&replybox, mUartGetcResp, reply);

  if (reply->result == -2)
    return -2;

  return reply->ch;
}

int ke_putc(int tid, int channel, char ch)
{
  MSG_INIT(req, mUartPutsReq);
  req.channel = channel;
  req.str_len = 1;
  req.str[0] = ch;
  req.blocking = false;
  MSGBOX_T(replybox, sizeof(struct mUartPutsResp));

  MSG_SEND(tid, &req, &replybox);

  if (replybox.len == -1)
    return -1;

  MSGBOX_CAST(&replybox, mUartPutsResp, reply);

  if (reply->result == -2)
    return -2;

  return 0;
}

int ke_puts(int tid, int channel, const char *str, int len)
{
  ASSERT_MSG(len <= M_PUTS_SZ, "ke_puts receives too large message\r\n");

  MSG_INIT(req, mUartPutsReq);
  req.channel = channel;
  req.str_len = len;
  util_memcpy(req.str, str, len);
  req.blocking = true;
  MSGBOX_T(replybox, sizeof(struct mUartPutsResp));

  MSG_SEND(tid, &req, &replybox);

  if (replybox.len == -1)
    return -1;

  MSGBOX_CAST(&replybox, mUartPutsResp, reply);

  if (reply->result == -2)
    return -2;

  return 0;
}
