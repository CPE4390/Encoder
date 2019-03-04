/* 
 * File:   DRV8825.h
 * Author: bemcg
 *
 * Created on February 21, 2019, 7:11 PM
 */

#ifndef DRV8825_H
#define	DRV8825_H

//Pins
// RJ0 = Step
// RJ1 = Dir
// RJ2 = M0
// RJ3 = M1  
// RJ4 = M2
// RJ5 = Reset //Optional
// RB1 = Fault
// Sleep and Reset must be connected to 3V3

#ifdef	__cplusplus
extern "C" {
#endif

    
    typedef enum {MODE_FULL = 0, MODE_HALF = 1, MODE_4 = 2, MODE_8 = 3, 
            MODE_16 = 4, MODE_32 = 5} StepMode;
    void InitDRV8825(StepMode mode);
    void SetMode(StepMode mode);
    void Step(int steps, unsigned int delay);

#define _XTAL_FREQ 32000000L
    
#ifdef	__cplusplus
}
#endif

#endif	/* DRV8825_H */

