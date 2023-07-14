#ifndef C_DASHBOARD_H
#define C_DASHBOARD_H

#include "rpi.h"
#include "util.h"
#include "printf.h"

static const char CURSOR_SAVE[] = "\0337";
static const char CURSOR_UNSAVE[] = "\0338";

static const char TERM_CLEAR[] = "\033[2J";
static const char TERM_RESET_CURSOR[] = "\033[H";
static const char TERM_HIDE_CURSOR[] = "\033[?25l";
static const char TERM_CLEAR_CURSOR[] = "\033[K";
static const char TERM_SAVE_CURSOR[] = "\0337";
static const char TERM_RESTORE_CURSOR[] = "\0338";
static const char TERM_DELETE_CURSOR_TO_EOL[] = "\033[K";
static const char TERM_DELETE_CURSOR_LINE[] = "\033[2K";

static const char SEP_LINE[] = "-------------\r\n";
static const char YELLOW_TXT[] = "\033[33m";
static const char BLUE_TXT[] = "\033[34m";
static const char RESET_TXT[] = "\033[0m";
static const char GREEN_TXT[] = "\033[32m";
static const char RED_TXT[] = "\033[31m";
static const char CYAN_TXT[] = "\033[36m";

#define DSB_ADD_MSG(CMD, MSG) \
  do                          \
  {                           \
    CMD += sprintf(CMD, MSG); \
  } while (0)

static inline char *dsb_set_cursor(char *cmd, uint16_t r, uint16_t c)
{
  // sprintf returns number of chars written to string
  return cmd + sprintf(cmd, "\033[%d;%dH", r, c);
}

static inline char *dsb_set_scrollable_region(char *cmd, uint16_t rs, uint16_t re)
{
  return cmd + sprintf(cmd, "\033[%d;%dr", rs, re);
}

static inline char *dsb_clear(char *cmd)
{
  DSB_ADD_MSG(cmd, TERM_CLEAR);
  DSB_ADD_MSG(cmd, RESET_TXT);
  return cmd;
}

static inline char *dsb_print_at_r_c(char *cmd, const char *msg, uint16_t r, uint16_t c)
{
  // DSB_ADD_MSG(cmd, TERM_SAVE_CURSOR);
  cmd = dsb_set_cursor(cmd, r, c);
  DSB_ADD_MSG(cmd, msg);
  // DSB_ADD_MSG(cmd, TERM_RESTORE_CURSOR);
  return cmd;
}

#endif