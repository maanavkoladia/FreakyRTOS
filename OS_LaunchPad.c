/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "OS_LaunchPad.h"
#include "../inc/tm4c123gh6pm.h"

#include <stdint.h>
#include <stddef.h>
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

void (*SW1Task)(void) = NULL;
void (*SW2Task)(void) = NULL;

int SW1_pri = INT32_MAX;
int SW2_pri = INT32_MAX;

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */


/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
void EdgeCounterPortF_Init(int32_t priority){                          
  SYSCTL_RCGCGPIO_R |= 0x00000020; // (a) activate clock for port F
  //FallingEdges = 0;             // (b) initialize counter
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
    
  GPIO_PORTF_DIR_R |=  0x0E;    // output on PF3,2,1 
  GPIO_PORTF_DIR_R &= ~0x11;    // (c) make PF4,0 in (built-in button)
    
  GPIO_PORTF_AFSEL_R &= ~0x1F;  //     disable alt funct on PF4,0
  GPIO_PORTF_DEN_R |= 0x1F;     //     enable digital I/O on PF4   
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF; // configure PF4 as GPIO
  GPIO_PORTF_AMSEL_R = 0;       //     disable analog functionality on PF
    
  GPIO_PORTF_PUR_R |= 0x11;     //     enable weak pull-up on PF4
    
  GPIO_PORTF_IS_R &= ~0x11;     // (d) PF4 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    //     PF4 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    //     PF4 falling edge event
    
  GPIO_PORTF_ICR_R = 0x11;      // (e) clear flag4
  GPIO_PORTF_IM_R |= 0x11;      // (f) arm interrupt on PF4 *** No IME bit as mentioned in Book ***
    
  //NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFF)|0x00A00000; // (g) priority 5
  NVIC_PRI7_R = (NVIC_PRI7_R & ~0x00E00000) | ((priority & 0x07) << 21);
  NVIC_EN0_R = 0x40000000;      // (h) enable interrupt 30 in NVIC
}

void OS_LaunchPad_Init(void){
    //TODO: eventaully deal with dynamic pf init

    //if a switch pri is a int32 max, then it is unsed

    if(SW1_pri == INT32_MAX && SW2_pri == INT32_MAX){
        EdgeCounterPortF_Init(7);
        return;    
    }

    int priority;

    if(SW1_pri < SW2_pri){
        priority = SW1_pri;
    }else {
        priority = SW2_pri;
    }

    EdgeCounterPortF_Init(priority);
}


int OS_AddSW1Task(void(*task)(void), uint32_t priority){
    SW1Task = task;
    SW1_pri = priority;
    return 1;
}

int OS_AddSW2Task(void(*task)(void), uint32_t priority){
    SW2Task = task;
    SW2_pri = priority;
    return 1;
}

void GPIOPortF_Handler(void){
    //figure out which is pressed
    int32_t flags = GPIO_PORTF_MIS_R & 0x11; //pull out pf4 and pf0
    GPIO_PORTF_ICR_R |= flags; //clear pf flags 
    if(flags == 0x1 && SW2Task != NULL){
        SW2Task();
    }else if(flags == 0x10 && SW1Task != NULL){
        SW1Task();
    }else{
        //TODO: add degub shit here
    }

}
