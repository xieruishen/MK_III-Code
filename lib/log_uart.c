#include "log_uart.h"

void LOG_init(void) {
    // TX is MOSI_A (PD3)
    // RX is SCK_A (PD4)

    // Enable UART & Tx/Rx
    LINCR |= _BV(LENA) | 0b111;

    // Set the baud rate to 9600
    LINBRR = 0x0C;
}

void LOG_chr(char data) {
    loop_until_bit_is_clear(LINSIR, LBUSY);
    LINDAT = data;
}

void LOG_println(char *data, uint8_t data_len) {
    // Send our data
    for (int i=0; i < data_len; i++) {
        loop_until_bit_is_clear(LINSIR, LBUSY);
        LINDAT = data[i];
    }

    // Send a newline
    loop_until_bit_is_clear(LINSIR, LBUSY);
    LINDAT = 0xa;

    loop_until_bit_is_clear(LINSIR, LBUSY);
    LINDAT = 0xd;
}
