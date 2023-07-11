#ifndef U_RPS_H
#define U_RPS_H

#include <stdbool.h>
#include <stdint.h>

enum RpsReqEnum
{
  RPS_SIGNUP,
  RPS_PLAY,
  RPS_QUIT,
};

enum RpsRespEnum
{
  GAME_STOP,
  GAME_WIN,
  GAME_LOSE,
  GAME_DRAW,
};

enum PlayerMove
{
  ROCK,
  PAPER,
  SCISSOR,
};

static inline const char *move_to_str(enum PlayerMove m)
{
  if (m == ROCK)
    return "Rock";
  if (m == PAPER)
    return "Paper";
  if (m == SCISSOR)
    return "Scissor";

  return "";
}

enum GameProgress
{
  WAIT_0,
  WAIT_1,
  QUIT_PENDING,
};

struct GameState
{
  enum GameProgress progress;
  int64_t player_1_tid;
  int64_t player_2_tid;
  enum PlayerMove player_1_move;
  enum PlayerMove player_2_move;
  struct GameState *next_pair;
  struct GameState *prev_pair;
};

bool signup(struct GameState *player_pair, int64_t tid);
void play(int64_t tid, enum PlayerMove);
void quit(int64_t tid);

void rps_server_main();

void rps_player_main_alice();

void rps_player_main_bob();

void rps_player_main_charles();

void rps_player_main_darwin();

#endif