#include <xc.h>
#include "LCD.h"
#include "DRV8825.h"

#pragma config FOSC=HSPLL
#pragma config WDTEN=OFF
#pragma config XINST=OFF

/*
Connections:
 * RB0 = Key
 * RB1 = S1
 * RB2 = S2
 */

//Encoder variables
volatile int requestedPosition = 0;
volatile char update = 0;
volatile char subStep;
volatile int posIncrement;
volatile int T1Ticks;
#define LONG_PRESS  16

volatile enum {
    IDLE = 0, WAIT_A_FALL, WAIT_B_FALL, WAIT_A_RISE, WAIT_B_RISE
} encoderState;

//Stepper variables
volatile StepMode currentStepMode = MODE_FULL;
int currentPosition = 0;

void InitPins(void);
void ConfigInterrupts(void);
void InitEncoder(void);

void main(void) {
    OSCTUNEbits.PLLEN = 1;
    LCDInit();
    LCDClear();
    InitPins();
    ConfigInterrupts();
    InitDRV8825(MODE_FULL);
    InitEncoder();
    lprintf(0, "Encoder+Stepper");
    lprintf(1, "Pos=%d Step=X%d", currentPosition, posIncrement);
    INTCONbits.GIE = 1;
    while (1) {
        INTCONbits.GIE = 0;
        int tempRequest = requestedPosition;
        INTCONbits.GIE = 1;
        int stepsNeeded = tempRequest - currentPosition;
        if (stepsNeeded != 0) {
            Step(stepsNeeded, 500);
            currentPosition += stepsNeeded;
            lprintf(1, "Pos=%d Step=X%d", currentPosition, posIncrement);
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
    subStep = 0;
    posIncrement = 10;
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
    T1CON = 0b00110000; //1:8 prescale, off
    TMR1H = 0;
    TMR1L = 0;
    PIE1bits.TMR1IE = 1;
    PIR1bits.TMR1IF = 0;
    INTCONbits.PEIE = 1;
}

void __interrupt(high_priority) HighIsr(void) {
    //Check the source of the interrupt
    if (INTCONbits.INT0IF == 1) {
        __delay_ms(1);
        if (PORTBbits.RB0 == 0) {
            if (posIncrement == 10) {
                posIncrement = 1;
            } else {
                posIncrement = 10;
            }
            TMR1H = 0;
            TMR1L = 0;
            PIR1bits.TMR1IF = 0;
            T1Ticks = 0;
            INTCON2bits.INTEDG0 = 1;
            T1CONbits.TMR1ON = 1;
            update = 1;
        } else {
            T1CONbits.TMR1ON = 0;
            PIR1bits.TMR1IF = 0;
            INTCON2bits.INTEDG0 = 0;
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
                    ++subStep;
                    if (subStep == 2) {
                        requestedPosition += posIncrement;
                        subStep = 0;
                        update = 1;
                    }
                    encoderState = IDLE;
                    INTCON2bits.INTEDG1 = 1;
                }
                break;
            case WAIT_A_RISE:
                if (INTCON2bits.INTEDG1 == 1) {
                    ++subStep;
                    if (subStep == 2) {
                        requestedPosition += posIncrement;
                        subStep = 0;
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
                    ++subStep;
                    if (subStep == 2) {
                        requestedPosition -= posIncrement;
                        subStep = 0;
                        update = 1;
                    }
                    encoderState = IDLE;
                    INTCON2bits.INTEDG2 = 1;
                }
                break;
            case WAIT_B_RISE:
                if (INTCON2bits.INTEDG2 == 1) {
                    ++subStep;
                    if (subStep == 2) {
                        requestedPosition -= posIncrement;
                        subStep = 0;
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
    if (PIR1bits.TMR1IF == 1) {
        ++T1Ticks;
        if (T1Ticks >= LONG_PRESS) {
            T1CONbits.TMR1ON = 0;
            requestedPosition = 0;
            posIncrement = 10;
            update = 1;
        }
        PIR1bits.TMR1IF = 0;
    }
}


