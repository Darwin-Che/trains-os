#include "lib/include/uapi.h"
#include "msg/msg.h"
#include "rps.h"
#include "lib/include/rpi.h"
#include "lib/include/printf.h"

void rps_player_main(int moveslen, enum PlayerMove *moves, const char *name)
{
  printf("%s init\r\n", name);
  int my_tid = ke_my_tid();
  printf("%s %d : starting\r\n", name, my_tid);
  // where is the server
  const char rps_server_name[] = "RPSServer";
  int rps_tid = ke_who_is(rps_server_name);
  assert(rps_tid >= 0);

  // Start the interaction
  MSG_INIT(req, mRpsRequest);
  MSGBOX_T(replybox, sizeof(struct mRpsResponse));

  // Sign up
  req.action = RPS_SIGNUP;
  printf("%s %d : sign up\r\n", name, my_tid);
  MSG_SEND(rps_tid, &req, &replybox);
  MSGBOX_CAST(&replybox, mRpsResponse, reply);
  assert(reply->result == RPS_SIGNUP);

  // Play moves
  for (int i = 0; i < moveslen; i += 1)
  {
    req.action = RPS_PLAY;
    req.player_choice = moves[i];
    const char *mov_str = move_to_str(req.player_choice);
    printf("%s %d : Submit %s\r\n", name, my_tid, mov_str);
    // uart_getc(0, 0);

    MSG_SEND(rps_tid, &req, &replybox);
    MSGBOX_CAST(&replybox, mRpsResponse, reply);

    if (reply->result == GAME_WIN)
      printf("%s %d : Get Win\r\n", name, my_tid);
    else if (reply->result == GAME_LOSE)
      printf("%s %d : Get Lose\r\n", name, my_tid);
    else if (reply->result == GAME_DRAW)
      printf("%s %d : Get Draw\r\n", name, my_tid);
    else if (reply->result == GAME_STOP)
    {
      printf("%s %d : Get Stop... Redo sign up\r\n", name, my_tid);

      req.action = RPS_SIGNUP;
      MSG_SEND(rps_tid, &req, &replybox);
      MSGBOX_CAST(&replybox, mRpsResponse, reply);
      assert(reply->result == RPS_SIGNUP);
      i -= 1; // retry the same move
    }
    else
    {
      printf("%s %d : Get invalid reply\r\n", name, my_tid);
      assert_fail();
    }
  }
  // Quit
  {
    req.action = RPS_QUIT;
    printf("%s %d : Submit Quit\r\n", name, my_tid);
    // uart_getc(0, 0);
    MSG_SEND(rps_tid, &req, &replybox);
    MSGBOX_CAST(&replybox, mRpsResponse, reply);
    assert(reply->result == GAME_STOP);
    printf("%s %d : Exit\r\n", name, my_tid);
  }
  return;
}

void rps_player_main_alice()
{
  enum PlayerMove moves[] = {ROCK, PAPER};
  rps_player_main(2, moves, "Alice");
}

void rps_player_main_bob()
{
  enum PlayerMove moves[] = {PAPER};
  rps_player_main(1, moves, "Bob");
}

void rps_player_main_charles()
{
  enum PlayerMove moves[] = {SCISSOR};
  rps_player_main(1, moves, "Charles");
}

void rps_player_main_darwin()
{
  enum PlayerMove moves[] = {SCISSOR, ROCK};
  rps_player_main(2, moves, "Darwin");
}