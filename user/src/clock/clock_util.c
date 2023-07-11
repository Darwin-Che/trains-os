#include "clock.h"
#include "kernel/uapi.h"
#include "msg/msg.h"

void clock_delay_main()
{
  int parent_tid = ke_my_parent_tid();
  int clock_tid = ke_who_is(NAME_CLOCK_SERVER);

  MSG_INIT(delay_req, mCkDelayTaskReq);
  MSGBOX_T(replybox, sizeof(struct mCkDelayTaskResp));

  while (1)
  {
    MSG_SEND(parent_tid, &delay_req, &replybox);
    if (replybox.len == -1)
      U_ASSERT_MSG(false, "clock_delay_main() to %d\r\n", parent_tid);
    MSGBOX_CAST(&replybox, mCkDelayTaskResp, reply);
    ke_delay_until(clock_tid, reply->until_tick);
  }
}