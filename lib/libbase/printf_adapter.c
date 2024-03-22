#include "printf.h"
#include "rpi.h"

void _putchar(char character)
{
  uart_putc(2, character);
}
