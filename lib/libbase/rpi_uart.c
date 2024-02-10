#include "rpi.h"

struct UART
{
  char DR;
  uint8_t DR_unused[3];
  uint32_t RSRECR;
  uint8_t RSRECR_unused[16];
  uint32_t FR;
  uint8_t FR_unused[4];
  uint32_t ILPR;
  uint32_t IBRD;
  uint32_t FBRD;
  uint32_t LCRH;
  uint32_t CR;
  uint32_t IFLS;
  uint32_t IMSC;
  uint32_t RIS;
  uint32_t MIS;
  uint32_t ICR;
  uint32_t DMACR;
  uint8_t DMACR_unused[52];
  uint32_t ITCR;
  uint32_t ITIP;
  uint32_t ITOP;
  uint32_t TDR;
};

// masks for specific fields in the UART registers
static const uint32_t UART_FR_RXFE = 0x10;
static const uint32_t UART_FR_TXFF = 0x20;
static const uint32_t UART_FR_RXFF = 0x40;
static const uint32_t UART_FR_TXFE = 0x80;

static const uint32_t UART_CR_UARTEN = 0x01;
static const uint32_t UART_CR_LBE = 0x80;
static const uint32_t UART_CR_TXE = 0x100;
static const uint32_t UART_CR_RXE = 0x200;
static const uint32_t UART_CR_RTS = 0x800;
static const uint32_t UART_CR_RTSEN = 0x4000;
static const uint32_t UART_CR_CTSEN = 0x8000;

static const uint32_t UART_LCRH_PEN = 0x2;
static const uint32_t UART_LCRH_EPS = 0x4;
static const uint32_t UART_LCRH_STP2 = 0x8;
static const uint32_t UART_LCRH_FEN = 0x10;
static const uint32_t UART_LCRH_WLEN_LOW = 0x20;
static const uint32_t UART_LCRH_WLEN_HIGH = 0x40;

static const uint32_t UARTCLK = 48000000;

/*
UART 0,2,3,4,5
*/

struct UART_PINS
{
  uint32_t setting;
  uint32_t resistor;
  uint32_t TX;
  uint32_t RX;
  // uint32_t CTS;
  // uint32_t RTS;
};

static const struct UART_PINS uart_pins_arr[6] = {
  {.TX = 14, .RX = 15, .setting = GPIO_ALTFN0, .resistor = GPIO_NONE},
  {0},
  {.TX = 0, .RX = 1, .setting = GPIO_ALTFN4, .resistor = GPIO_NONE},
  {.TX = 4, .RX = 5, .setting = GPIO_ALTFN4, .resistor = GPIO_NONE},
  {.TX = 8, .RX = 9, .setting = GPIO_ALTFN4, .resistor = GPIO_NONE},
  {.TX = 12, .RX = 13, .setting = GPIO_ALTFN4, .resistor = GPIO_NONE},
};

volatile struct UART * uart_from_id(uint8_t id)
{
  return (volatile struct UART *) (0xfe201000 + 0x200 * (uint32_t) id);
}

static void uart_setup_gpio(uint8_t id)
{
  const struct UART_PINS * pins = &uart_pins_arr[id];
  setup_gpio(pins->TX, pins->setting, pins->resistor);
  setup_gpio(pins->RX, pins->setting, pins->resistor);
}

void uart_init(uint8_t id, uint32_t baudrate)
{
  // Setup GPIO pins
  uart_setup_gpio(id);

  volatile struct UART * uart = uart_from_id(id);
  // Turn of uart
  uint32_t cr_state = uart->CR;
  uart->CR = 0;

  // to avoid floating point, this computes 64 times the required baud divisor
  uint32_t baud_divisor = (uint32_t)((((uint64_t)UARTCLK)*4)/baudrate);

  // Clear all interrupts
  // uart->ICR = 0x7ff;
  uart->ICR = 0;

  // set the line control registers: 8 bit, no parity, 1 stop bit, FIFOs enabled
  uart->LCRH = UART_LCRH_WLEN_HIGH | UART_LCRH_WLEN_LOW | UART_LCRH_FEN;
  // set the baud rate
  uart->IBRD = baud_divisor >> 6;
  uart->FBRD = baud_divisor & 0x3f;
  uart->CR = cr_state | UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE; // Enable RX and TX and FIFO
}

void uart_putc(uint8_t id, char c) {
  volatile struct UART * uart = uart_from_id(id);
  
  // wait until we can send
  do {
    asm volatile("nop");
  } while(uart->FR & 0x20);
  // write the character to the buffer
  uart->DR=c;
}

char uart_getc(uint8_t id) {
  volatile struct UART * uart = uart_from_id(id);
  char r;

  // wait until something is in the buffer
  do{
    asm volatile("nop");
  } while (uart->FR & 0x10);

  // read it and return
  r = uart->DR;
  // convert carrige return to newline
  return r=='\r'?'\n':r;
}