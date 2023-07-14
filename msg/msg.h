#ifndef M_MSG_H
#define M_MSG_H

#include "lib/include/assert.h"
#include "lib/include/printf.h"
#include "lib/include/sys_const.h"
#include "lib/include/util.h"
#include "user/include/clock.h"
#include "user/include/ke_print.h"

struct mEmptyMsg {
  char type;
};

/* Dashboard */
#define M_PUTS_SZ 1010

// only used by ke_print
struct mDsbDebugReq {
  char type;
  char msg[M_PUTS_SZ];
};

struct mDsbDebugResp {
  char type;
};

/* Clock */

struct mCkNotifyReq {
  char type;
};

struct mCkNotifyResp {
  char type;
};

struct mCkTimeReq {
  char type;
};

struct mCkDelayReq {
  char type;
  int ticks;
};

struct mCkDelayUntilReq {
  char type;
  int tick;
};

struct mCkResp {
  char type;
  int tick;
};

struct mCkDelayTaskReq {
  char type;
};

struct mCkDelayTaskResp {
  char type;
  int until_tick;
};

/* NameServer */

struct mNsRegisterReq {
  char type;
  char name[C_MAX_NS_NAME_SIZE];
};

struct mNsWhoisReq {
  char type;
  char name[C_MAX_NS_NAME_SIZE];
};

struct mNsWhoisResp {
  char type;
  int tid;
};

/* K3 clients */

struct mK3Req {
  char type;
};

struct mK3Resp {
  char type;
  int delay_ticks;
  int delay_cnt;
};

/* Rock Paper Scissors */

struct mRpsRequest {
  char type;
  char action;
  char player_choice;
};

struct mRpsResponse {
  char type;
  char result;
};

/* Uart Server (Internal) */
struct mUartNotifierReq {
  char type;
  int intr_id;
};

struct mUartNotifierResp {
  char type;
};

struct mUartTermInputReq {
  char type;
};

struct mUartTermOutputReq {
  char type;
};

struct mUartMarklinInputReq {
  char type;
};

struct mUartMarklinOutputReq {
  char type;
};

struct mUartMarklinCtsReq {
  char type;
};

struct mUartEmptyResp {
  char type;
};

/* Uart Server API */

struct mUartGetcReq {
  char type;
  int channel;
};

struct mUartGetcResp {
  char type;
  char ch;
  int result;
};

struct mUartPutsReq {
  char type;
  int channel;
  int str_len;
  bool blocking;
  char str[M_PUTS_SZ]; // so that sizeof(struct) = 1024
};

struct mUartPutsResp {
  char type;
  int result;
};

/* MSG Type Inference */

#define MSG_TYPE_ENUM(MSG_TT) MSG_TT##Enum

enum mMsgTypeEnum {
  /* empty */
  MSG_TYPE_ENUM(mEmptyMsg),
  /* Dashboard */
  MSG_TYPE_ENUM(mDsbDebugReq),
  MSG_TYPE_ENUM(mDsbDebugResp),
  /* Clock */
  MSG_TYPE_ENUM(mCkNotifyReq),
  MSG_TYPE_ENUM(mCkNotifyResp),
  MSG_TYPE_ENUM(mCkTimeReq),
  MSG_TYPE_ENUM(mCkDelayReq),
  MSG_TYPE_ENUM(mCkDelayUntilReq),
  MSG_TYPE_ENUM(mCkResp),
  MSG_TYPE_ENUM(mCkDelayTaskReq),
  MSG_TYPE_ENUM(mCkDelayTaskResp),
  /* Name Server */
  MSG_TYPE_ENUM(mNsRegisterReq),
  MSG_TYPE_ENUM(mNsWhoisReq),
  MSG_TYPE_ENUM(mNsWhoisResp),
  /* K3 clients */
  MSG_TYPE_ENUM(mK3Req),
  MSG_TYPE_ENUM(mK3Resp),
  /* Rock paper scissor */
  MSG_TYPE_ENUM(mRpsRequest),
  MSG_TYPE_ENUM(mRpsResponse),
  /* Uart Server (internal) */
  MSG_TYPE_ENUM(mUartNotifierReq),
  MSG_TYPE_ENUM(mUartNotifierResp),
  MSG_TYPE_ENUM(mUartTermInputReq),
  MSG_TYPE_ENUM(mUartTermOutputReq),
  MSG_TYPE_ENUM(mUartMarklinInputReq),
  MSG_TYPE_ENUM(mUartMarklinOutputReq),
  MSG_TYPE_ENUM(mUartMarklinCtsReq),
  MSG_TYPE_ENUM(mUartEmptyResp),
  /* Uart Server */
  MSG_TYPE_ENUM(mUartGetcReq),
  MSG_TYPE_ENUM(mUartGetcResp),
  MSG_TYPE_ENUM(mUartPutsReq),
  MSG_TYPE_ENUM(mUartPutsResp),
};

#define MSG_TYPE_OF_HELPER(MSG_TT) struct MSG_TT : MSG_TYPE_ENUM(MSG_TT)

#define MSG_TYPE_OF(MSG)                                                       \
  _Generic(                                                                    \
      (MSG), MSG_TYPE_OF_HELPER(mCkNotifyReq),                                 \
      MSG_TYPE_OF_HELPER(mCkNotifyResp), MSG_TYPE_OF_HELPER(mCkTimeReq),       \
      MSG_TYPE_OF_HELPER(mCkDelayReq), MSG_TYPE_OF_HELPER(mCkDelayUntilReq),   \
      MSG_TYPE_OF_HELPER(mCkResp), MSG_TYPE_OF_HELPER(mCkDelayTaskReq),        \
      MSG_TYPE_OF_HELPER(mCkDelayTaskResp),                                    \
      MSG_TYPE_OF_HELPER(mNsRegisterReq), MSG_TYPE_OF_HELPER(mNsWhoisReq),     \
      MSG_TYPE_OF_HELPER(mNsWhoisResp), MSG_TYPE_OF_HELPER(mK3Req),            \
      MSG_TYPE_OF_HELPER(mK3Resp), MSG_TYPE_OF_HELPER(mRpsRequest),            \
      MSG_TYPE_OF_HELPER(mRpsResponse), MSG_TYPE_OF_HELPER(mUartNotifierReq),  \
      MSG_TYPE_OF_HELPER(mUartNotifierResp),                                   \
      MSG_TYPE_OF_HELPER(mUartTermInputReq),                                   \
      MSG_TYPE_OF_HELPER(mUartTermOutputReq),                                  \
      MSG_TYPE_OF_HELPER(mUartMarklinInputReq),                                \
      MSG_TYPE_OF_HELPER(mUartMarklinOutputReq),                               \
      MSG_TYPE_OF_HELPER(mUartMarklinCtsReq),                                  \
      MSG_TYPE_OF_HELPER(mUartGetcReq), MSG_TYPE_OF_HELPER(mUartGetcResp),     \
      MSG_TYPE_OF_HELPER(mUartPutsReq), MSG_TYPE_OF_HELPER(mUartPutsResp),     \
      MSG_TYPE_OF_HELPER(mUartEmptyResp), MSG_TYPE_OF_HELPER(mEmptyMsg))

/*
MSG API

Send
  MSG_INIT(msg, mSomeMsg); // struct mSomeMsg msg;
  // fill the content of msg
  MSGBOX_T(replybox, sizeof(mExpectedMsg));
  MSG_SEND(&msg, &replybox);
  if (MSGBOX_IS(&replybox, mExpectedMsg)) {
    MSGBOX_CAST(&replybox, mExpectedMsg, reply); // struct mExpectedMsg *reply =
(struct mExpectedMsg *) replybox.msg;
  }

Recv
  MSGBOX_T(recvbox, sizeof(mExpectedMsg));
  MSG_RECV(&recvbox);
  if (MSGBOX_IS(&recvbox, mExpectedMssg)) {
    MSGBOX_CAST(&replybox, mExpectedMsg, reply); // struct mExpectedMsg *reply =
(struct mExpectedMsg *) replybox.msg;
  }

Reply
  MSG_INIT(msg, mSomeMsg); // struct mSomeMsg msg;
  // fill the content of msg
  MSG_REPLY(&msg);
*/

#define MSG_FILL_TYPE(MSG) (MSG)->type = MSG_TYPE_OF(*MSG)

#define MSG_INIT(MSG, MSG_TT)                                                  \
  struct MSG_TT MSG;                                                           \
  MSG_FILL_TYPE(&MSG)

#define MSGBOX_T(BOX, MSG_MAX_LEN)                                             \
  struct {                                                                     \
    int len;                                                                   \
    int _pad;                                                                  \
    char msg[MSG_MAX_LEN];                                                     \
  } BOX

#define MSGBOX_IS(MB, MSG_TT)                                                  \
  (((struct mEmptyMsg *)((MB)->msg))->type == MSG_TYPE_ENUM(MSG_TT))

#define MSGBOX_CAST(MB, MSG_TT, MSG)                                           \
  U_ASSERT_MSG(MSGBOX_IS(MB, MSG_TT) && ((MB)->len == sizeof(struct MSG_TT)),  \
               "MSGBOX_CAST TYPE len=%d type=%d\r\n", (MB)->len,               \
               ((struct mEmptyMsg *)((MB)->msg))->type);                       \
  struct MSG_TT *MSG = (struct MSG_TT *)((MB)->msg);                           \
  (void)MSG

#define MSG_SEND(TID, MSG, REPLYBOX)                                           \
  do {                                                                         \
    (REPLYBOX)->len = ke_send(TID, (char *)MSG, sizeof(*MSG), (REPLYBOX)->msg, \
                              sizeof((REPLYBOX)->msg));                        \
    U_ASSERT_MSG((REPLYBOX)->len <= (int)sizeof((REPLYBOX)->msg),              \
                 "MSG_SEND LEN\r\n");                                          \
  } while (0)

#define MSG_RECV(TID_PTR, RECVBOX)                                             \
  do {                                                                         \
    (RECVBOX)->len = ke_recv(TID_PTR, (RECVBOX)->msg, sizeof((RECVBOX)->msg)); \
    U_ASSERT_MSG((RECVBOX)->len <= (int)sizeof((RECVBOX)->msg),                \
                 "MSG_RECV LEN\r\n");                                          \
  } while (0)

#define MSG_REPLY(TID, MSG)                                                    \
  do {                                                                         \
    int len = ke_reply(TID, (char *)MSG, sizeof(*MSG));                        \
    U_ASSERT_MSG(len <= (int)sizeof(*MSG), "MSG_REPLY LEN\r\n");               \
  } while (0)

#endif