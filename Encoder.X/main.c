#include <xc.h>
#include "LCD.h"

#pragma config FOSC=HSPLL
#pragma config WDTEN=OFF
#pragma config XINST=OFF

/*
Connections:
 * RB0 = Key
 * RB1 = S1
 * RB2 = S2
 */

volatile int position = 0;
volatile char update = 0;
volatile char step;
volatile enum {
    IDLE = 0, WAIT_A_FALL, WAIT_B_FALL, WAIT_A_RISE, WAIT_B_RISE
} encoderState;

void InitPins(void);
void ConfigInterrupts(void);
void InitEncoder(void);

void main(void) {
    long i;
    OSCTUNEbits.PLLEN = 1;
    LCDInit();
    LCDClear();
    InitPins();
    ConfigInterrupts();
    InitEncoder();
    INTCONbits.GIE = 1;
    lprintf(0, "Encoder");
    lprintf(1, "Position=%d", position);
    while (1) {
        if (update) {
            update = 0;
            lprintf(1, "Position=%d", position);
        }
    }
}

void InitEncoder(void) {
    TRISB |= 0b00000110; //A and B inputs
    __delay_ms(20);
    if (PORTBbits.RB1 == 1) {
        INTCON2bits.INTEDG1 = 0;
    } else {
        INTCON2bits.INTEDG1 = 1;
    }
    if (PORTBbits.RB2 == 1) {
        INTCON2bits.INTEDG2 = 0;
    } else {
        INTCON2bits.INTEDG2 = 1;
    }
    INTCON3bits.INT1IE = 1;
    INTCON3bits.INT1IF = 0;
    INTCON3bits.INT2IE = 1;
    INTCON3bits.INT2IF = 0;
    encoderState = IDLE;
    step = 0;
}

void InitPins(void) {
    LATD = 0; //LED's are outputs
    TRISD = 0; //Turn off all LED's
    LATB = 0;
    TRISB = 0;
    TRISB |= 0b00000001; //RB0 input
}

void ConfigInterrupts(void) {
    RCONbits.IPEN = 0; //no priorities.  This is the default.
    INTCON2bits.INTEDG0 = 0;
    INTCONbits.INT0IE = 1;
    INTCONbits.INT0IF = 0;
}

void __interrupt(high_priority) HighIsr(void) {
    //Check the source of the interrupt
    if (INTCONbits.INT0IF == 1) {
        //source is INT0
        __delay_ms(1);
        if (PORTBbits.RB0 == 0) {
            position = 0;
            update = 1;
        }
        INTCONbits.INT0IF = 0; //clear the flag
    }
    if (INTCON3bits.INT1IF == 1) {
        __delay_us(50); //Debounce
        switch (encoderState) {
            case IDLE:
                if (INTCON2bits.INTEDG1 == 0) {
                    encoderState = WAIT_B_FALL;
                    INTCON2bits.INTEDG2 = 0;
                    INTCON2bits.INTEDG1 = 1;
                } else {
                    encoderState = WAIT_B_RISE;
                    INTCON2bits.INTEDG2 = 1;
                    INTCON2bits.INTEDG1 = 0;
                }
                break;
            case WAIT_A_FALL:
                if (INTCON2bits.INTEDG1 == 0) {
                    ++step;
                    if (step == 2) {
                        ++position;
                        step = 0;
                        update = 1;
                    }
                    encoderState = IDLE;
                    INTCON2bits.INTEDG1 = 1;
                }
                break;
            case WAIT_A_RISE:
                if (INTCON2bits.INTEDG1 == 1) {
                    ++step;
                    if (step == 2) {
                        ++position;
                        step = 0;
                        update = 1;
                    }
                    encoderState = IDLE;
                    INTCON2bits.INTEDG1 = 0;
                }
                break;
            case WAIT_B_FALL:
            case WAIT_B_RISE:
                encoderState = IDLE;
                INTCON2bits.INTEDG1 ^= 1;
                break;
        }
        INTCON3bits.INT1IF = 0; //clear the flag
    }
    if (INTCON3bits.INT2IF == 1) {
        __delay_us(50); //Debounce
        switch (encoderState) {
            case IDLE:
                if (INTCON2bits.INTEDG2 == 0) {
                    encoderState = WAIT_A_FALL;
                    INTCON2bits.INTEDG1 = 0;
                    INTCON2bits.INTEDG2 = 1;
                } else {
                    encoderState = WAIT_A_RISE;
                    INTCON2bits.INTEDG1 = 1;
                    INTCON2bits.INTEDG2 = 0;
                }
                break;
            case WAIT_B_FALL:
                if (INTCON2bits.INTEDG2 == 0) {
                    ++step;
                    if (step == 2) {
                        --position;
                        step = 0;
                        update = 1;
                    }
                    encoderState = IDLE;
                    INTCON2bits.INTEDG2 = 1;
                }
                break;
            case WAIT_B_RISE:
                if (INTCON2bits.INTEDG2 == 1) {
                    ++step;
                    if (step == 2) {
                        --position;
                        step = 0;
                        update = 1;
                    }
                    encoderState = IDLE;
                    INTCON2bits.INTEDG2 = 0;
                }
                break;
            case WAIT_A_FALL:
            case WAIT_A_RISE:
                encoderState = IDLE;
                INTCON2bits.INTEDG2 ^= 1;
                break;
        }
        INTCON3bits.INT2IF = 0;
    }
}


