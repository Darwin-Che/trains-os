#ifndef K_MAILBOX_H
#define K_MAILBOX_H

#include "lib/include/ring_buffer.h"
#include "sys_const.h"

struct kTaskDsp;

// A ring buffer of waiting senders (td pointers plz)
struct kMailbox {
  RB_TYPE(struct kTaskDsp *, USER_STACK_ARRAY_CNT)
  sender_waitqueue;

  const char *out_msg;
  size_t out_msg_len;

  int *in_tid;
  char *in_buf;
  size_t in_buf_len;
};

// void k_mbox_init(struct kMailbox *mbox);

// void k_mbox_send_init(struct kMailbox *mbox, const char *msg, size_t msglen,
// char *reply, size_t rplen); void k_mbox_recv_init(struct kMailbox *mbox, char
// *msg, size_t msglen, int *tid);

// void k_mbox_add_send_queue(struct kMailbox *mbox, struct kTaskDsp *sender);
// struct kTaskDsp *k_mbox_pop_send_queue(struct kMailbox *mbox);

// void k_mbox_copy_msg(struct kMailbox *to, struct kMailbox *from);

static inline void k_mbox_init(struct kMailbox *mbox) {
  util_memset(mbox, 0x0, sizeof(struct kMailbox));
  RB_INIT(&mbox->sender_waitqueue, USER_STACK_ARRAY_CNT);
}

static inline void k_mbox_send_init(struct kMailbox *mbox, const char *msg,
                                    size_t msglen, char *reply, size_t rplen) {
  mbox->out_msg = msg;
  mbox->out_msg_len = msglen;
  mbox->in_buf = reply;
  mbox->in_buf_len = rplen;
}

static inline void k_mbox_recv_init(struct kMailbox *mbox, char *msg,
                                    size_t msglen, int *tid) {
  mbox->in_buf = msg;
  mbox->in_buf_len = msglen;
  mbox->in_tid = tid;
}

static inline void k_mbox_add_send_queue(struct kMailbox *mbox,
                                         struct kTaskDsp *sender) {
  struct kTaskDsp **slot;
  RB_PUSH_PRE(&mbox->sender_waitqueue, slot);
  *slot = sender;
}

static inline struct kTaskDsp *k_mbox_pop_send_queue(struct kMailbox *mbox) {
  struct kTaskDsp **slot;
  RB_PEEK(&mbox->sender_waitqueue, slot);
  if (slot == NULL) {
    return NULL;
  }
  struct kTaskDsp *td = *slot;
  RB_POP(&mbox->sender_waitqueue);
  return td;
}

static inline void k_mbox_copy_msg(struct kMailbox *to, struct kMailbox *from) {
  size_t copy_len = MIN(to->in_buf_len, from->out_msg_len);
  util_memcpy(to->in_buf, from->out_msg, copy_len);
}

#endif