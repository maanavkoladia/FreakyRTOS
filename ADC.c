/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */

#include <stdint.h>
#include <stdbool.h>

#include "ADCSWTrigger.h"
#include "../inc/Timer4A.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/LPF.h"
#include "OS.h"
#include "../inc/IRDistance.h"
#include "UART0int.h"
#include "ST7735.h"
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
int32_t ADCdata,FilterOutput,Distance;
static uint32_t FilterWork;
char dataOutBufADC[30];
// periodic task
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

#define ADC_SAMPLING_RATE 1
#define Timer4A_PERIOD SYSCLK_PERIOD/ADC_SAMPLING_RATE
void PeriodicTask_DAStask(void);
uint32_t ADC_In(void);
bool showADCData_flag;
/* ================================================== */
/*                    MAIN FUNCTION                   */
/* ================================================== */
void PeriodicTask_DAStask(void){  // runs at 10Hz in background
  //GPIO_PORTF_DATA_R ^= 0x04;
  ADCdata = ADC_In();  // channel set when calling ADC_Init
  if(showADCData_flag){
    //printf("ADC VAl: %d\n", ADCdata);
    ST7735_Message(0, 0, "Sampled Data: ", ADCdata);
  }
  //GPIO_PORTF_DATA_R ^= 0x04;
  //FilterOutput = Median(ADCdata); // 3-wide median filter
  //Distance = IRDistance_Convert(FilterOutput,0);
  //FilterWork++;        // calculation finished
  //GPIO_PORTF_DATA_R ^= 0x04;
}

int ADC_Init(uint32_t channelNum){
// put your Lab 1 code here
  if(channelNum > 11){
    //printf("Invalid channelNum");
    return false;
  }
  
  ADC0_InitSWTriggerSeq3(channelNum);

  Timer4A_Init(PeriodicTask_DAStask, Timer4A_PERIOD, 1); 
   
  //printf("ADC initing on channel: %d\n\r", channelNum);
    showADCData_flag = false;
  //showADCData_flag = false;
  return true;
}
// software start sequencer 3 and return 12 bit ADC result
uint32_t ADC_In(void){
// put your Lab 1 code here
    return ADC0_InSeq3();
}

void ShowADCData(void){
  showADCData_flag = true;
}

void StopShowingADCData(void){
  showADCData_flag = false;
}
