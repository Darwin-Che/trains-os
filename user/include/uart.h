#ifndef U_UART_H
#define U_UART_H

// Use 0 for A, 1 for B
#define WAIT_FOR_CTS 2

void uart_notifier_main();
void uart_server_term_main();
void uart_server_marklin_main();

#endif