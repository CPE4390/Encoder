#include <xc.h>
#include "LCD.h"

#pragma config FOSC=HSPLL
#pragma config WDTEN=OFF
#pragma config XINST=OFF

/*
Connections:
 * RB0 = 
 * RB1 = S1
 * RB2 = S2
 */

volatile int position = 0;
volatile char update = 0;

void InitPins(void);
void ConfigInterrupts(void);

void main(void) {
    long i;
    OSCTUNEbits.PLLEN = 1;
    LCDInit();
    LCDClear();
    InitPins();
    ConfigInterrupts();
    lprintf(0, "Encoder");
    lprintf(1, "Position=%d", position);
    while (1) {
        if (update) {
            update = 0;
            lprintf(1, "Position=%d", position);
        }
    }
}

void InitPins(void) {
    LATD = 0; //LED's are outputs
    TRISD = 0; //Turn off all LED's
    
    TRISB = 0b00000111; //RB0, RB1, RB2 inputs
}

void ConfigInterrupts(void) {

    RCONbits.IPEN = 0; //no priorities.  This is the default.

    INTCON2bits.INTEDG0 = 0;
    INTCONbits.INT0IE = 1;
    INTCONbits.INT0IF = 0;
    INTCONbits.GIE = 1;
}

void __interrupt(high_priority) HighIsr(void) {
    unsigned char rx;
    //Check the source of the interrupt
    if (INTCONbits.INT0IF == 1) {
        //source is INT0

        INTCONbits.INT0IF = 0; //clear the flag
    }
}


