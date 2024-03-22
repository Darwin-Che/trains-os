#include "rpi.h"
#include "assert.h"

#define BUFFER_NTM_MAX_LENGTH 256

struct AUX
{
  uint32_t IRQ;
  uint32_t ENABLES;
};

struct SPI
{
  uint32_t CNTL0; // Control register 0
  uint32_t CNTL1; // Control register 1
  uint32_t STAT;  // Status
  uint32_t PEEK;  // Peek
  uint32_t _unused[4];
  uint32_t IO_REGa;     // Data
  uint32_t IO_REGb;     // Data
  uint32_t IO_REGc;     // Data
  uint32_t IO_REGd;     // Data
  uint32_t TXHOLD_REGa; // Extended Data
  uint32_t TXHOLD_REGb; // Extended Data
  uint32_t TXHOLD_REGc; // Extended Data
  uint32_t TXHOLD_REGd; // Extended Data
};

static char *const AUX_BASE = (char *)(GPIO_BASE + 0x15000);

static volatile struct AUX *const aux = (struct AUX *)(AUX_BASE);
static volatile struct SPI *const spi[] = {(struct SPI *)(AUX_BASE + 0x80), (struct SPI *)(AUX_BASE + 0xC0)};

void setup_gpio(uint32_t pin, uint32_t setting, uint32_t resistor)
{
  uint32_t reg = pin / 10;
  uint32_t shift = (pin % 10) * 3;
  uint32_t status = gpio->GPFSEL[reg]; // read status
  status &= ~(7u << shift);            // clear bits
  status |= (setting << shift);        // set bits
  gpio->GPFSEL[reg] = status;          // write back

  reg = pin / 16;
  shift = (pin % 16) * 2;
  status = gpio->PUP_PDN_CNTRL_REG[reg]; // read status
  status &= ~(3u << shift);              // clear bits
  status |= (resistor << shift);         // set bits
  gpio->PUP_PDN_CNTRL_REG[reg] = status; // write back
}

void init_gpio()
{
  setup_gpio(18, GPIO_ALTFN4, GPIO_NONE);
  setup_gpio(19, GPIO_ALTFN4, GPIO_NONE);
  setup_gpio(20, GPIO_ALTFN4, GPIO_NONE);
  setup_gpio(21, GPIO_ALTFN4, GPIO_NONE);

  // uarts' interrupt line
  setup_gpio(24, GPIO_INPUT, GPIO_NONE);
  gpio->GPLEN0 |= (0x1 << 24);
}

static const uint32_t SPI_CNTL0_DOUT_HOLD_SHIFT = 12;
static const uint32_t SPI_CNTL0_CS_SHIFT = 17;
static const uint32_t SPI_CNTL0_SPEED_SHIFT = 20;

static const uint32_t SPI_CNTL0_POSTINPUT = 0x00010000;
static const uint32_t SPI_CNTL0_VAR_CS = 0x00008000;
static const uint32_t SPI_CNTL0_VAR_WIDTH = 0x00004000;
static const uint32_t SPI_CNTL0_Enable = 0x00000800;
static const uint32_t SPI_CNTL0_In_Rising = 0x00000400;
static const uint32_t SPI_CNTL0_Clear_FIFOs = 0x00000200;
static const uint32_t SPI_CNTL0_Out_Rising = 0x00000100;
static const uint32_t SPI_CNTL0_Invert_CLK = 0x00000080;
static const uint32_t SPI_CNTL0_SO_MSB_FST = 0x00000040;
static const uint32_t SPI_CNTL0_MAX_SHIFT = 0x0000003F;

static const uint32_t SPI_CNTL1_CS_HIGH_SHIFT = 8;

static const uint32_t SPI_CNTL1_Keep_Input = 0x00000001;
static const uint32_t SPI_CNTL1_SI_MSB_FST = 0x00000002;
static const uint32_t SPI_CNTL1_Done_IRQ = 0x00000040;
static const uint32_t SPI_CNTL1_TX_EM_IRQ = 0x00000080;

static const uint32_t SPI_STAT_TX_FIFO_MASK = 0xFF000000;
static const uint32_t SPI_STAT_RX_FIFO_MASK = 0x00FF0000;
static const uint32_t SPI_STAT_TX_FULL = 0x00000400;
static const uint32_t SPI_STAT_TX_EMPTY = 0x00000200;
static const uint32_t SPI_STAT_RX_FULL = 0x00000100;
static const uint32_t SPI_STAT_RX_EMPTY = 0x00000080;
static const uint32_t SPI_STAT_BUSY = 0x00000040;
static const uint32_t SPI_STAT_BIT_CNT_MASK = 0x0000003F;

void init_spi(uint32_t channel)
{
  uint32_t reg = aux->ENABLES;
  reg |= (2 << channel);
  aux->ENABLES = reg;
  spi[channel]->CNTL0 = SPI_CNTL0_Clear_FIFOs;
  uint32_t speed = (700000000 / (2 * 0x400000)) - 1; // for maximum bitrate 0x400000
  spi[channel]->CNTL0 = (speed << SPI_CNTL0_SPEED_SHIFT) | SPI_CNTL0_VAR_WIDTH | SPI_CNTL0_Enable | SPI_CNTL0_In_Rising | SPI_CNTL0_SO_MSB_FST;
  spi[channel]->CNTL1 = SPI_CNTL1_SI_MSB_FST;
}

static void spi_send_recv(uint32_t channel, const char *sendbuf, size_t sendlen, char *recvbuf, size_t recvlen)
{
  size_t sendidx = 0;
  size_t recvidx = 0;
  while (sendidx < sendlen || recvidx < recvlen)
  {
    uint32_t data = 0;
    size_t count = 0;

    // prepare write data
    for (; sendidx < sendlen && count < 24; sendidx += 1, count += 8)
    {
      data |= (sendbuf[sendidx] << (16 - count));
    }
    data |= (count << 24);

    // always need to write something, otherwise no receive
    while (spi[channel]->STAT & SPI_STAT_TX_FULL)
      asm volatile("yield");
    if (sendidx < sendlen)
    {
      spi[channel]->TXHOLD_REGa = data; // keep chip-select active, more to come
    }
    else
    {
      spi[channel]->IO_REGa = data;
    }

    // read transaction
    while (spi[channel]->STAT & SPI_STAT_RX_EMPTY)
      asm volatile("yield");
    data = spi[channel]->IO_REGa;

    // process data, if needed, assume same byte count in transaction
    // QUESTION: WHY?
    size_t max = (recvlen - recvidx) * 8;
    if (count > max)
      count = max;
    for (; count > 0; recvidx += 1, count -= 8)
    {
      recvbuf[recvidx] = (data >> (count - 8)) & 0xFF;
    }
  }
}

void spi_uart_write_register(size_t spiChannel, size_t uartChannel, char reg, char data)
{
  char req[2] = {0};
  req[0] = (uartChannel << UART_CHANNEL_SHIFT) | (reg << UART_ADDR_SHIFT);
  req[1] = data;
  spi_send_recv(spiChannel, req, 2, NULL, 0);
}

char spi_uart_read_register(size_t spiChannel, size_t uartChannel, char reg)
{
  char req[2] = {0};
  char res[2] = {0};
  req[0] = (uartChannel << UART_CHANNEL_SHIFT) | (reg << UART_ADDR_SHIFT) | UART_READ_ENABLE;
  spi_send_recv(spiChannel, req, 2, res, 2);
  return res[1];
}

static void spi_uart_init_channel(size_t spiChannel, size_t uartChannel, size_t baudRate, bool twostopbit)
{
  // set baud rate
  spi_uart_write_register(spiChannel, uartChannel, UART_LCR, UART_LCR_DIV_LATCH_EN);
  uint32_t bauddiv = 14745600 / (baudRate * 16);
  spi_uart_write_register(spiChannel, uartChannel, UART_DLH, (bauddiv & 0xFF00) >> 8);
  spi_uart_write_register(spiChannel, uartChannel, UART_DLL, (bauddiv & 0x00FF));

  if (twostopbit)
  {
    // This is Marklin Box
    spi_uart_write_register(spiChannel, uartChannel, UART_LCR, 0xBF);
    spi_uart_write_register(spiChannel, uartChannel, UART_EFR, UART_EFR_ENABLE_ENHANCED_FNS);
    spi_uart_write_register(spiChannel, uartChannel, UART_LCR, 0x7);
    spi_uart_write_register(spiChannel, uartChannel, UART_FCR, UART_FCR_RX_FIFO_RESET | UART_FCR_TX_FIFO_RESET | UART_FCR_FIFO_EN);
    // spi_uart_write_register(spiChannel, uartChannel, UART_FCR, UART_FCR_RX_FIFO_RESET | UART_FCR_TX_FIFO_RESET);
  }
  else
  {
    // This is Terminal
    // set serial byte configuration: 8 bit, no parity, 1 stop bit
    spi_uart_write_register(spiChannel, uartChannel, UART_LCR, 0x3);
    spi_uart_write_register(spiChannel, uartChannel, UART_FCR, UART_FCR_RX_FIFO_RESET | UART_FCR_TX_FIFO_RESET | UART_FCR_FIFO_EN);
  }

  // Initialize the registers
  spi_uart_write_register(spiChannel, uartChannel, UART_IER, 0); // Disable all interrupts

  // clear and enable fifos, (wait since clearing fifos takes time)
  for (int i = 0; i < 65535; ++i)
    asm volatile("yield");
}

void init_uart(uint32_t spiChannel)
{
  spi_uart_write_register(spiChannel, 0, UART_IOControl, UART_IOControl_RESET); // resets both channels
  spi_uart_init_channel(spiChannel, 0, 115200, false);
  spi_uart_init_channel(spiChannel, 1, 2400, true);
}

char spi_uart_getc(size_t spiChannel, size_t uartChannel)
{
  while (spi_uart_read_register(spiChannel, uartChannel, UART_RXLVL) == 0)
    asm volatile("yield");
  return spi_uart_read_register(spiChannel, uartChannel, UART_RHR);
}

bool spi_uart_getc_nonblock(size_t spiChannel, size_t uartChannel, char *c)
{
  if (spi_uart_read_register(spiChannel, uartChannel, UART_RXLVL) == 0)
    return false;
  *c = spi_uart_read_register(spiChannel, uartChannel, UART_RHR);
  return true;
}

void spi_uart_putc(size_t spiChannel, size_t uartChannel, char c)
{
  while (spi_uart_read_register(spiChannel, uartChannel, UART_TXLVL) == 0)
    asm volatile("yield");
  spi_uart_write_register(spiChannel, uartChannel, UART_THR, c);
}

bool spi_uart_write_is_blocked(size_t spiChannel, size_t uartChannel)
{
  return spi_uart_read_register(spiChannel, uartChannel, UART_TXLVL) == 0;
}

bool spi_uart_putc_nonblock(size_t spiChannel, size_t uartChannel, char c)
{
  if (spi_uart_read_register(spiChannel, uartChannel, UART_TXLVL) == 0)
    return false;
  spi_uart_write_register(spiChannel, uartChannel, UART_THR, c);
  return true;
}

void spi_uart_puts(size_t spiChannel, size_t uartChannel, const char *buf, size_t blen)
{
  static const size_t max = 32;
  char temp[max];
  temp[0] = (uartChannel << UART_CHANNEL_SHIFT) | (UART_THR << UART_ADDR_SHIFT);
  size_t tlen = spi_uart_read_register(spiChannel, uartChannel, UART_TXLVL);
  if (tlen > max)
    tlen = max;
  for (size_t bidx = 0, tidx = 1;;)
  {
    if (tidx < tlen && bidx < blen)
    {
      temp[tidx] = buf[bidx];
      bidx += 1;
      tidx += 1;
    }
    else
    {
      spi_send_recv(spiChannel, temp, tidx, NULL, 0);
      if (bidx == blen)
        break;
      tlen = spi_uart_read_register(spiChannel, uartChannel, UART_TXLVL);
      if (tlen > max)
        tlen = max;
      tidx = 1;
    }
  }
}

// Expects a null terminated string only, will assert fail if over
void spi_uart_puts_ntm(size_t spiChannel, size_t uartChannel, const char *buf)
{
  static const size_t max = 32;

  char temp[max];
  temp[0] = (uartChannel << UART_CHANNEL_SHIFT) | (UART_THR << UART_ADDR_SHIFT);
  size_t tlen = spi_uart_read_register(spiChannel, uartChannel, UART_TXLVL);
  if (tlen > max)
    tlen = max;
  for (size_t bidx = 0, tidx = 1;;)
  {
    if (tidx < tlen && buf[bidx] != '\0')
    {
      temp[tidx] = buf[bidx];
      bidx += 1;
      tidx += 1;
      ASSERT_MSG(bidx <= BUFFER_NTM_MAX_LENGTH, "Buffer too large, likely not null terminated. Current blen: %d", bidx);
    }
    else
    {
      spi_send_recv(spiChannel, temp, tidx, NULL, 0);
      if (tidx != tlen)
        break;
      tlen = spi_uart_read_register(spiChannel, uartChannel, UART_TXLVL);
      if (tlen > max)
        tlen = max;
      tidx = 1;
    }
  }
}

// Debug Test
void spi_uart_test()
{
  spi_uart_puts_ntm(0, 0, "UART TEST!!!\r\n");
}