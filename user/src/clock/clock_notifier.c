#include "msg/msg.h"
#include "kernel/uapi.h"

void clock_notifier_main()
{
  int cstid = ke_my_parent_tid();

  MSG_INIT(req, mCkNotifyReq);
  MSGBOX_T(replybox, sizeof(struct mCkNotifyResp));

  while (1)
  {
    ke_await_event(KE_INTR_TIMER);

    MSG_SEND(cstid, &req, &replybox);
    assert(MSGBOX_IS(&replybox, mCkNotifyResp));
  }
}