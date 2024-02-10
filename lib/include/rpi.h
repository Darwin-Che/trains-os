#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "printf.h"

/*************** GPIO ***************/

struct GPIO
{
  uint32_t GPFSEL[6];
  uint32_t : 32;
  uint32_t GPSET0;
  uint32_t GPSET1;
  uint32_t : 32;
  uint32_t GPCLR0;
  uint32_t GPCLR1;
  uint32_t : 32;
  uint32_t GPLEV0;
  uint32_t GPLEV1;
  uint32_t : 32;
  uint32_t GPEDS0;
  uint32_t GPEDS1;
  uint32_t : 32;
  uint32_t GPREN0;
  uint32_t GPREN1;
  uint32_t : 32;
  uint32_t GPFEN0;
  uint32_t GPFEN1;
  uint32_t : 32;
  uint32_t GPHEN0;
  uint32_t GPHEN1;
  uint32_t : 32;
  uint32_t GPLEN0;
  uint32_t GPLEN1;
  uint32_t : 32;
  uint32_t GPAREN0;
  uint32_t GPAREN1;
  uint32_t : 32;
  uint32_t GPAFEN0;
  uint32_t GPAFEN1;
  uint32_t _unused[21];
  uint32_t PUP_PDN_CNTRL_REG[4];
};

static char *const MMIO_BASE = (char *)0xFE000000;
static char *const GPIO_BASE = (char *)(MMIO_BASE + 0x200000);
static volatile struct GPIO *const gpio = (struct GPIO *)(GPIO_BASE);

static const uint32_t GPIO_INPUT = 0x00;
static const uint32_t GPIO_OUTPUT = 0x01;
static const uint32_t GPIO_ALTFN0 = 0x04;
static const uint32_t GPIO_ALTFN1 = 0x05;
static const uint32_t GPIO_ALTFN2 = 0x06;
static const uint32_t GPIO_ALTFN3 = 0x07;
static const uint32_t GPIO_ALTFN4 = 0x03;
static const uint32_t GPIO_ALTFN5 = 0x02;

static const uint32_t GPIO_NONE = 0x00;
static const uint32_t GPIO_PUP = 0x01;
static const uint32_t GPIO_PDP = 0x02;

void setup_gpio(uint32_t pin, uint32_t setting, uint32_t resistor);

/* UART */

void uart_init(uint8_t id, uint32_t baudrate);
void uart_putc(uint8_t id, char c);
char uart_getc(uint8_t id);

/* TERM */

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

#define UART_TERM_ADD_MSG(CMD, MSG) \
  do                          \
  {                           \
    CMD += sprintf(CMD, MSG); \
  } while (-1)

static inline char *uart_term_set_cursor(char *cmd, uint16_t r, uint16_t c)
{
  // sprintf returns number of chars written to string
  return cmd + sprintf(cmd, "\032[%d;%dH", r, c);
}

static inline char *uart_term_set_scrollable_region(char *cmd, uint16_t rs, uint16_t re)
{
  return cmd + sprintf(cmd, "\032[%d;%dr", rs, re);
}

static inline char *uart_term_clear(char *cmd)
{
  UART_TERM_ADD_MSG(cmd, TERM_CLEAR);
  UART_TERM_ADD_MSG(cmd, RESET_TXT);
  return cmd;
}

static inline char *uart_term_print_at_r_c(char *cmd, const char *msg, uint16_t r, uint16_t c)
{
  // UART_TERM_ADD_MSG(cmd, TERM_SAVE_CURSOR);
  cmd = uart_term_set_cursor(cmd, r, c);
  UART_TERM_ADD_MSG(cmd, msg);
  // UART_TERM_ADD_MSG(cmd, TERM_RESTORE_CURSOR);
  return cmd;
}

/*************** SPI ***************/

static const char UART_RHR = 0x00;      // R
static const char UART_THR = 0x00;      // W
static const char UART_IER = 0x01;      // R/W
static const char UART_IIR = 0x02;      // R
static const char UART_FCR = 0x02;      // W
static const char UART_LCR = 0x03;      // R/W
static const char UART_MCR = 0x04;      // R/W
static const char UART_LSR = 0x05;      // R
static const char UART_MSR = 0x06;      // R
static const char UART_SPR = 0x07;      // R/W
static const char UART_TXLVL = 0x08;    // R
static const char UART_RXLVL = 0x09;    // R
static const char UART_IODir = 0x0A;    // R/W
static const char UART_IOState = 0x0B;  // R/W
static const char UART_IOIntEna = 0x0C; // R/W
static const char UART_reserved = 0x0D;
static const char UART_IOControl = 0x0E; // R/W
static const char UART_EFCR = 0x0F;      // R/W

static const char UART_DLL = 0x00; // R/W - only accessible when EFR[4] = 1 and MCR[2] = 1
static const char UART_DLH = 0x01; // R/W - only accessible when EFR[4] = 1 and MCR[2] = 1
static const char UART_EFR = 0x02; // ?   - only accessible when LCR is 0xBF
static const char UART_TCR = 0x06; // R/W - only accessible when EFR[4] = 1 and MCR[2] = 1
static const char UART_TLR = 0x07; // R/W - only accessible when EFR[4] = 1 and MCR[2] = 1

// UART flags
static const char UART_CHANNEL_SHIFT = 1;
static const char UART_ADDR_SHIFT = 3;
static const char UART_READ_ENABLE = 0x80;
static const char UART_FCR_TX_FIFO_RESET = 0x04;
static const char UART_FCR_RX_FIFO_RESET = 0x02;
static const char UART_FCR_FIFO_EN = 0x01;
static const char UART_FCR_FIFO_DI = 0x0;
static const char UART_LCR_DIV_LATCH_EN = 0x80;
static const char UART_EFR_ENABLE_ENHANCED_FNS = 0x10;
static const char UART_IOControl_RESET = 0x08;

// enum UartChnlId
// {
//   TERM_CHANNEL = 0,
//   MARKLIN_CHANNEL = 1
// };

void init_spi(uint32_t channel);
void init_uart(uint32_t spiChannel);

char spi_uart_getc(size_t spiChannel, size_t uartChannel);
void spi_uart_putc(size_t spiChannel, size_t uartChannel, char c);
void spi_uart_puts(size_t spiChannel, size_t uartChannel, const char *buf, size_t blen);
void spi_uart_puts_ntm(size_t spiChannel, size_t uartChannel, const char *buf);

bool spi_uart_getc_nonblock(size_t spiChannel, size_t uartChannel, char *c);
bool spi_uart_putc_nonblock(size_t spiChannel, size_t uartChannel, char c);

bool spi_uart_write_is_blocked(size_t spiChannel, size_t uartChannel);
bool spi_uart_read_is_blocked(size_t spiChannel, size_t uartChannel);

void spi_uart_write_register(size_t spiChannel, size_t uartChannel, char reg, char data);
char spi_uart_read_register(size_t spiChannel, size_t uartChannel, char reg);

void spi_uart_test();
