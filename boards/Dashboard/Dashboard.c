/**********************************************************
This is the base document for getting started with writing
firmware for OEM.
TODO delete before submitting
**********************************************************/

/*
Header:
    Explain what this project does as overview
Author:
    @author
*/

/*----- Includes -----*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "can_api.h"

/*----- Macro Definitions -----*/
// LEDs
#define EXT_LED_ORANGE              PB5
#define EXT_LED_GREEN               PB4
#define PORT_EXT_LED_ORANGE         PORTB
#define PORT_EXT_LED_GREEN          PORTB
#define LED1_PIN                    PB6
#define PORT_LED1                   PORTB


// I/O
#define START_PIN                   PB2
#define PORT_START                  PORTB

// CAN Positions
#define CAN_START_BUTTON            0


// gFlag positions
#define STATUS_START                1
#define BRAKE_PRESSED               2
#define TSMS_CLOSED                 3


#define UPDATE_STATUS               0


/*----- Global Variables -----*/
volatile uint8_t gFlag = 0x00;  // Global Flag
uint8_t gCAN_MSG[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // CAN Message
uint8_t can_recv_msg[8] = {};

// Timer counters
uint8_t gClock_prescale = 0x00;  // Used for update timer



/*----- Interrupt(s) -----*/
// *pg 76 of datasheet*
ISR(CAN_INT_vect) {
    // Check first board (Brake Light)
    CANPAGE = (0 << MOBNB0);
    if(bit_is_set(CANSTMOB, RXOK)) {
        can_recv_msg[0] = CANMSG;   // brake
        can_recv_msg[1] = CANMSG;   // brake_pos
        can_recv_msg[2] = CANMSG;   // bspd
        can_recv_msg[3] = CANMSG;   // hvd
        can_recv_msg[4] = CANMSG;   // tsms
        can_recv_msg[5] = CANMSG;   // left_e_stop
        can_recv_msg[6] = CANMSG;   // right_e_stop
        can_recv_msg[7] = CANMSG;   // main_fuse

        // can_recv_msg[0] = msg;
        // can_recv_msg[1] = 0x99;

        if(can_recv_msg[0] == 0xFF) {
            gFlag |= _BV(BRAKE_PRESSED);           //trip flag
        } else {
            gFlag &= ~_BV(BRAKE_PRESSED);          //reset flag
        }

        if(can_recv_msg[5] == 0xFF) {
            gFlag |= _BV(TSMS_CLOSED);
        } else {
            gFlag &= ~_BV(TSMS_CLOSED);
        }

        //Setup to Receive Again
        CANSTMOB = 0x00;
        CAN_wait_on_receive(0, CAN_ID_BRAKE_LIGHT, CAN_LEN_BRAKE_LIGHT, CAN_IDM_single);
    }
}

ISR(PCINT0_vect) {
    /*
    Standard Pin Change Interupt
    Covers Interrupts: Start Button
    */
    if(PORT_START, START_PIN) {
        gFlag |= _BV(STATUS_START);
    } else {
        gFlag &= ~_BV(STATUS_START);
    }
}

ISR(TIMER0_COMPA_vect) {
    // Only send CAN msgs every 20 cycles
    if(gClock_prescale > 20) {
        gFlag |= _BV(UPDATE_STATUS);
        gClock_prescale = 0;
    }
    gClock_prescale++;
}


/*----- Functions -----*/
void initTimer(void) {
    TCCR0A = _BV(WGM01);    // Set up 8-bit timer in CTC mode
    TCCR0B = 0x05;          // clkio/1024 prescaler
    TIMSK0 |= _BV(OCIE0A);
    OCR0A = 0xFF;
}

void checkShutdownState(void)   {
    /*
    -Check if bits are set
        -IF they are, set CAN list position to 0xFF
        -ELSE do set CAN list position to 0x00
    */
}

void updateStateFromFlags(void) {
    /*
    Based off the state of the flag(s), update components and send CAN
    */
    if(bit_is_set(gFlag, STATUS_START)) {
        gCAN_MSG[CAN_START_BUTTON] = 0xFF;
    } else {
        gCAN_MSG[CAN_START_BUTTON] = 0x00;
    }
}

//TODO any other functionality goes here


/*----- MAIN -----*/
int main(void){
    /*
    -Set up I/O
    -Set up CAN timer (done)
    -Initialize external libraries (if applicable)
    -Wait on CAN
    -Infinite loop checking shutdown state!
    */

    sei();                  // Enable Interrupts

    DDRB |= _BV(LED1_PIN) | _BV(EXT_LED_ORANGE) | _BV(EXT_LED_GREEN);


    /* Setup interrupt registers */
    PCICR |= _BV(PCIE0);
    PCMSK0 |= _BV(PCINT2);


    // CAN Enable
    CAN_init(CAN_ENABLED);
    CAN_wait_on_receive(0, CAN_ID_BRAKE_LIGHT, CAN_LEN_BRAKE_LIGHT, CAN_IDM_single);

    initTimer();

    gFlag |= _BV(UPDATE_STATUS);        // Read ports

    while(1) {
        if(bit_is_set(gFlag, UPDATE_STATUS)) {
            PORT_EXT_LED_ORANGE ^= _BV(EXT_LED_ORANGE);     // Blink Orange LED for timing check
            PORT_LED1 ^= _BV(LED1_PIN);

            // updateStateFromFlags();


            // if(bit_is_set(gFlag, BRAKE_PRESSED) && bit_is_set(gFlag, TSMS_CLOSED)) {
            //     CAN_transmit(0, CAN_ID_DASHBOARD, CAN_LEN_DASHBOARD, gCAN_MSG);
            // }
            if(bit_is_set(gFlag, TSMS_CLOSED)) {
                PORT_EXT_LED_GREEN ^= _BV(EXT_LED_GREEN);
            }
            gFlag &= ~_BV(UPDATE_STATUS);  // Clear Flag
        }

        // Do continuously for now
        updateStateFromFlags();

        if(bit_is_set(gFlag, BRAKE_PRESSED) && bit_is_set(gFlag, TSMS_CLOSED)) {
            CAN_transmit(0, CAN_ID_DASHBOARD, CAN_LEN_DASHBOARD, gCAN_MSG);
        }
    }
}
