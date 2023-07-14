#include "kernel/uapi.h"
#include "lib/include/utlist.h"
#include "rps.h"
#include "msg/msg.h"
#include "lib/include/printf.h"
#include "lib/include/utlist.h"
#include "lib/include/rpi.h"

struct Game
{
  struct GameState all_pairs[100];
  struct GameState *games_list;
  struct GameState *free_list;
};

void game_init(struct Game *game)
{
  game->free_list = NULL;
  game->games_list = NULL;

  // Support 100 games at the same time
  for (int i = 100 - 1; i >= 0; i -= 1)
    DL_PREPEND2(game->free_list, &game->all_pairs[i], prev_pair, next_pair);
}

bool tid_in_game(struct GameState *pair, int tid)
{
  return pair->player_1_tid != tid && pair->player_2_tid != tid;
}

void quit_game(struct Game *game, struct GameState *pair)
{
  MSG_INIT(resp, mRpsResponse);
  resp.result = GAME_STOP;

  printf("Server: Player1 (%d) Reply Quit\r\n", pair->player_1_tid);
  // uart_getc(0, 0);
  MSG_REPLY(pair->player_1_tid, &resp);

  printf("Server: Player2 (%d) Reply Quit\r\n", pair->player_2_tid);
  // uart_getc(0, 0);
  MSG_REPLY(pair->player_2_tid, &resp);

  printf("Server: Game between Player1 (%d) and Player2 (%d) has ended\r\n", pair->player_1_tid, pair->player_2_tid);
  uart_getc(0, 0);
  // delete the pair
  DL_DELETE2(game->games_list, pair, prev_pair, next_pair);
  DL_APPEND2(game->free_list, pair, prev_pair, next_pair);
}

enum RpsRespEnum calc_game(enum PlayerMove m1, enum PlayerMove m2)
{
  if (m1 == m2)
    return GAME_DRAW;

  if (m1 == ROCK)
  {
    if (m2 == SCISSOR)
      return GAME_WIN;
    if (m2 == PAPER)
      return GAME_LOSE;
  }

  if (m1 == PAPER)
  {
    if (m2 == ROCK)
      return GAME_WIN;
    if (m2 == SCISSOR)
      return GAME_LOSE;
  }

  if (m1 == SCISSOR)
  {
    if (m2 == PAPER)
      return GAME_WIN;
    if (m2 == ROCK)
      return GAME_DRAW;
  }
  // should not reach here
  printf("Calc Game Error! Quiting!\r\n");
  assert_fail();
  return GAME_DRAW;
}

void rps_server_main()
{
  // Register the server
  char rps_server_name[] = "RPSServer";
  int success_val = ke_register_as(rps_server_name);
  assert(success_val == 0);

  struct Game game;
  game_init(&game);

  int waiting_tid;
  bool waiting = false;

  int req_tid;
  MSGBOX_T(reqbox, sizeof(struct mRpsRequest));
  MSG_INIT(resp, mRpsResponse);

  while (1)
  {
    MSG_RECV(&req_tid, &reqbox);
    MSGBOX_CAST(&reqbox, mRpsRequest, request);

    if (request->action == RPS_SIGNUP)
    {
      printf("Server: Player %d comes\r\n", req_tid);
      // uart_getc(0, 0);

      if (waiting)
      {
        // forms a pair, start playing
        struct GameState *pair = game.free_list;
        assert(pair != NULL);
        DL_DELETE2(game.free_list, pair, prev_pair, next_pair);
        pair->player_1_tid = waiting_tid;
        pair->player_2_tid = req_tid;
        pair->progress = WAIT_0;
        printf("Server: Creating Game : %d vs %d\r\n", pair->player_1_tid, pair->player_2_tid);
        // uart_getc(0, 0);
        DL_PREPEND2(game.games_list, pair, prev_pair, next_pair);
        waiting = false;
        // Unblock both player
        resp.result = RPS_SIGNUP;
        MSG_REPLY(pair->player_1_tid, &resp);
        MSG_REPLY(pair->player_2_tid, &resp);
      }
      else
      {
        waiting_tid = req_tid;
        waiting = true;
      }
    }
    else if (request->action == RPS_PLAY)
    {
      // find the pair
      struct GameState *pair;
      LL_SEARCH2(game.games_list, pair, req_tid, tid_in_game, next_pair);
      assert(pair != NULL);

      const char *move_str;
      if (pair->player_1_tid == req_tid)
      {
        pair->player_1_move = request->player_choice;
        move_str = move_to_str(pair->player_1_move);
        printf("Server: Player1 (%d) Submitted %s\r\n", pair->player_1_tid, move_str);
        // uart_getc(0, 0);
      }
      else if (pair->player_2_tid == req_tid)
      {
        pair->player_2_move = request->player_choice;
        move_str = move_to_str(pair->player_2_move);
        printf("Server: Player2 (%d) Submitted %s\r\n", pair->player_2_tid, move_str);
        // uart_getc(0, 0);
      }

      if (pair->progress == WAIT_0)
      {
        pair->progress = WAIT_1;
      }
      else if (pair->progress == WAIT_1)
      {
        pair->progress = WAIT_0;
        printf("Server: Both Moves Ready, Calcing and Sending Result to %d, %d\r\n",
               pair->player_1_tid, pair->player_2_tid);
        uart_getc(0, 0);

        // respond the game
        resp.result = calc_game(pair->player_1_move, pair->player_2_move);
        // printf("Server: Sending First Result to %d\r\n", pair->player_1_tid);
        // uart_getc(0, 0);
        MSG_REPLY(pair->player_1_tid, &resp);

        if (resp.result == GAME_WIN)
          resp.result = GAME_LOSE;
        else if (resp.result == GAME_LOSE)
          resp.result = GAME_WIN;
        else
          resp.result = GAME_DRAW;
        // printf("Server: Sending Second Result to %d\r\n", pair->player_2_tid);
        // uart_getc(0, 0);
        MSG_REPLY(pair->player_2_tid, &resp);

        printf("Server: Ready to receive more moves\r\n");
        // uart_getc(0, 0);
      }
      else if (pair->progress == QUIT_PENDING)
      {
        quit_game(&game, pair);
      }
    }
    else if (request->action == RPS_QUIT)
    {
      // search for the pair
      struct GameState *pair;
      LL_SEARCH2(game.games_list, pair, req_tid, tid_in_game, next_pair);

      if (pair->player_1_tid == req_tid)
      {
        printf("Server: Player1 (%d) Submitted Quit\r\n", pair->player_1_tid);
        // uart_getc(0, 0);
      }
      else if (pair->player_2_tid == req_tid)
      {
        printf("Server: Player2 (%d) Submitted Quit\r\n", pair->player_2_tid);
        // uart_getc(0, 0);
      }

      if (pair->progress == QUIT_PENDING || pair->progress == WAIT_1)
      {
        quit_game(&game, pair);
      }
      else
      {
        pair->progress = QUIT_PENDING;
      }
    }
    else
    {
      printf("Server: Received invalid request\r\n");
      assert_fail();
    }
  }
}
