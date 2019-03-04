#include <xc.h>
#include "DRV8825.h"

void inline __delay(unsigned int d);

void InitDRV8825(StepMode mode) {
    LATJ &= 0b1100000; //No step, direction = fwd, mode = full step, hold in reset
    TRISJ &= 0b11000000; //RJ0 - RJ5 outputs
    TRISB |= 0b00000010; //RB1 input 
    __delay_us(1);
    LATJbits.LATJ5 = 1; //Release from reset
    __delay_us(1);
    SetMode(mode);
}

void SetMode(StepMode mode) {
    mode &= 0b00000111;
    mode <<= 2;
    LATJ &= 0b11100011;
    LATJ |= mode;
    __delay_us(1);
}

void Step(int steps, unsigned int delay) {
    if (steps < 0) {
        LATJbits.LATJ1 = 1;
        steps = -steps;
    } else {
        LATJbits.LATJ1 = 0;
    }
    while (steps > 0) {
        LATJbits.LATJ0 = 1;
        __delay_us(2);
        LATJbits.LATJ0 = 0;
        __delay(delay);
        --steps;
    }
}

void inline __delay(unsigned int d) {
    while (d) {
        __delay_us(2);
        --d;
    }
}