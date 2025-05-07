// *************os.c**************
// EE44//5M/EE380L.6 Labs 1, 2, 3, and 4 
// High-level OS functions
// Students will implement these functions as part of Lab
// Runs on LM4F120/TM4C123
// Jonathan W. Valvano 
// Jan 12, 2020, valvano@mail.utexas.edu

// timer0/2 unused
//timer 1 used for os clock time
//timer 3 and 4 used for periodtasks 
//timer5 used for sleep checking every MS, and global MS time
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

//#include "printf.h"
#include "printf.h"
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "../inc/PLL.h"
//#include "../inc/Timer0A.h"
//#include "../inc/Timer1A.h"
//#include "../inc/Timer2A.h"
#include "../inc/Timer3A.h"
#include "../inc/Timer4A.h"
#include "../inc/Timer5A.h"
//#include "../inc/ADCT0ATrigger.h"
//#include "../inc/LaunchPad.h"

#include "OS_LaunchPad.h"
#include "OS.h"
#include "OS_Lists.h"
#include "UART0int.h"
#include "ST7735.h"
#include "OS_CLI_Commands/CLI_Commands.h"

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS (GLOBALS)   */
/* ================================================== */
//#define NUM_SEMA_4S 10
//sema4_t sema4Pool[NUM_SEMA_4S];
uint32_t globalTimeMS = 0;

TCB_t* RunPt = NULL;
TCB_t* nextPtCS = NULL;

OS_FIFO_t* OS_FreePtrs_FIFO = NULL;
#define OS_FREEPTRS_FIFO_SIZE 100

int totalThreadCount = 0;
int totalProcessCount = 0; 
uint16_t nextThreadID = 1;

uint16_t nextPID = 1;

uint8_t periodicThreadCount = 0;
void (*User_PeriodicTask1)(void) = NULL;
void (*User_PeriodicTask2)(void) = NULL;
uint32_t UserTask1_Period;
uint32_t UserTask2_Period;

int (*SerialOutPtr)(char) = NULL;
sema4_t serialMutex;

sema4_t printf_sema4;

#define ENV_SYMBOL_TABLE_SIZE 1


#define PD0  (*((volatile uint32_t *)0x40007004))
#define PD1  (*((volatile uint32_t *)0x40007008))
#define PD2  (*((volatile uint32_t *)0x40007010))
#define PD3  (*((volatile uint32_t *)0x40007020))


int OS_Running = 0;
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */
extern void ContextSwitch(void);

void OS_Time_Init(void);
void Timer1_Init_Global_Clock_Count(void);
void StartupDelay(void);
void Init_VectorTable(void);
int OS_KillProcess(PCB_t* pcb);

void Task_OS_Bookkeeping(void);
/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ====================================== ============ */

//make sure run ptr is right, and assign next tcb ptr for context switch
void SysTick_Handler(void) {
    GPIO_PORTF_DATA_R ^= 0x02;
    Scheduler();
    ContextSwitch();
    //GPIO_PORTF_DATA_R ^= 0x04;
    //GPIO_PORTF_DATA_R ^= 0x04;  
    //GPIO_PORTF_DATA_R ^= 0x04;
} 

//TODO:this is supposed to be like sart crit/end crit, make sure i didnt fuck this up
unsigned long OS_LockScheduler(void){
    uint32_t status = (NVIC_ST_CTRL_R & NVIC_ST_CTRL_ENABLE);
    NVIC_ST_CTRL_R &= ~(NVIC_ST_CTRL_ENABLE);
    return  status;
}

void OS_UnLockScheduler(unsigned long previous){
  NVIC_ST_CTRL_R |= previous;
}

void SysTick_Init(unsigned long period){
    NVIC_ST_CTRL_R = 0;                   // disable SysTick during setup
    NVIC_ST_RELOAD_R = period-1;  // maximum reload value
    NVIC_ST_CURRENT_R = 0;                // any write to current clears it
    
    //setting pendsv and syspris
    SYSPRI3 = (SYSPRI3 & ~(0x7 << 29)) | (6 << 29);

    // Clear bits 23-21 and set them to 7 (111)
    SYSPRI3 = (SYSPRI3 & ~(0x7 << 21)) | (7 << 21);    

    NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE | NVIC_ST_CTRL_CLK_SRC | NVIC_ST_CTRL_INTEN;
}

void SVC_PRI_INIT(uint8_t pri){
    //clear bits 31-29
    SYSPRI2 &= ~(0x7 << 29);

    //set bits 31-29 to the pri
    SYSPRI2 |= (pri & 0x7) << 29;
}

void OS_Init(void){
    StartupDelay();
    DisableInterrupts();
    PLL_Init(Bus80MHz);

    UART_Init();

    ST7735_InitR(INITR_REDTAB); // LCD initialization
    SVC_PRI_INIT(6);
    
    //eDisk_Init(0);
    //printf("UART and LCD Inited");


    OS_ListsInit();
    SerialOutPtr = &UART_OutChar;
    //OS_FreePtrs_FIFO = OS_Fifo_Init(OS_FREEPTRS_FIFO_SIZE);


    OS_InitSemaphore(&serialMutex, 1);
    OS_InitSemaphore(&printf_sema4, 1);
    CLI_Commands_Init();
    
    //OS_AddThread(Task_OS_Bookkeeping, 128, 0);
    //printf("OS_Init Success\n\r");
}

void OS_Launch(uint32_t theTimeSlice){
    SysTick_Init(theTimeSlice);
    OS_LaunchPad_Init();
    OS_Time_Init();

    OS_ClearMsTime();
    
    Timer5A_Init(PeriodicTask_CheckSleepPool, TIME_1MS, 6);


    if(totalThreadCount < 1){//if there are no thread kill os
        printf("No Threads added\n");
        StartupDelay();
        OSStartHang();
    }
    
    RunPt = GetFirstTask(RunPt);
    Scheduler();
    //OS_Running = 1;
    StartOS();
}

void OS_Sleep(uint32_t sleepTime) {

    //TODO: deal with when to turn on/off scheduler
    if(sleepTime > MAX_SLEEP_MS){
        return;
    } 

    // Lock the scheduler to ensure atomic operation
    
    //roll over shoudl be fine as long as it doenst do it twice and time isnt reset

    RunPt-> timeToWakeUpMS = OS_MsTime() + sleepTime + 1;

    TCB_t* singMeALullabyGoober = RunPt;
 

    int status = StartCritical();
    OS_RemoveTCBFromActivePool(singMeALullabyGoober);

    OS_AddTCBToSleepPool(singMeALullabyGoober);
    
    Scheduler();
    ContextSwitch();
    EndCritical(status);
    
    //ensures doesnt keep running
    //while(singMeALullabyGoober->timeToWakeUpMS != OS_MsTime()){}

}

void OS_Kill(void){
    int status = StartCritical();
    
    totalThreadCount--;

    //PCB_t* pcb = RunPt->parentProcess;

    //if(pcb != NULL){
    //    pcb->threadCount--;
    //    if(pcb->threadCount == 0){
    //        totalProcessCount--;
    //        Free(pcb->codeSection);
    //        Free(pcb->dataSection);
    //        pcb->PID = -1;//makr as dead
    //        printf("Process Just Killed\n\r");
    //    }
    //}

    OS_RemoveTCBFromActivePool(RunPt);

    RunPt->isDead = 1;
    

    Scheduler();
    ContextSwitch();
    EndCritical(status);
    //add support for killing pcb

    while(1){}
}

void OS_Suspend(void){
    Scheduler();
    ContextSwitch();
}

int OS_AddThread(void(*task)(void), uint32_t stackSize, uint32_t priority){
    //TODO: check if there is a PCB based on current runtpt or paretn thread
    int status = StartCritical();

    if(totalThreadCount > NUMTHREADS  || stackSize > STACKSIZE){
        EndCritical(status);
        return OS_FAIL;
    }
    
    TCB_t* openTCB_ptr = NULL;

    //if(RunPt->parentProcess != NULL && OS_Running){
    //    //called from a process
    //    openTCB_ptr = GetOpenTCB(task, stackSize, PROCESS_CALLER, RunPt->parentProcess);
    //}else{
    //    openTCB_ptr = GetOpenTCB(task, stackSize, THREAD_CALLER, NULL);
    //}
    
    openTCB_ptr = GetOpenTCB(task, stackSize, THREAD_CALLER, NULL);

    if(openTCB_ptr == NULL){
        EndCritical(status);
        return OS_FAIL;
    }

    //initialize tcb params for a active thread
    openTCB_ptr->priority = priority;
    openTCB_ptr->timeToWakeUpMS = 0;
    openTCB_ptr->id = nextThreadID;

    //if(RunPt->parentProcess != NULL && OS_Running){
    //    openTCB_ptr->parentProcess = RunPt->parentProcess;
    //    openTCB_ptr->parentProcess->threadCount++;
    //}else{
    //    openTCB_ptr->parentProcess = NULL;
    //}

    OS_AddTCBToActivePool(openTCB_ptr);

    totalThreadCount++;
    nextThreadID++;
    EndCritical(status);
    return OS_SUCCESS;
}


#ifdef CALCULATE_TASK_JITTER

#define JITTERSIZE_TASK1 64
#define JITTERSIZE_TASK2 64

int32_t MaxJitter_Task1 = 0;             // largest time jitter between interrupts in usec
uint32_t JitterHistogram_Task1[JITTERSIZE_TASK1] = {0,};

int32_t MaxJitter_Task2 = 0;             // largest time jitter between interrupts in usec
uint32_t JitterHistogram_Task2[JITTERSIZE_TASK2] = {0,};

uint32_t ComputeJitter(uint32_t lastTime, uint32_t currTime, uint32_t period) {
    uint32_t diff, jitter;

    // Handle timer overflow (wrap-around case)
    if (currTime >= lastTime) {
        diff = currTime - lastTime;
    } else {
        // Timer overflow occurred: compute wrap-around difference
        diff = (0xFFFFFFFF - lastTime) + currTime + 1;
    }

    // Compute the absolute deviation from expected period
    if (diff > period) {
        jitter = diff - period;
    } else {
        jitter = period - diff;
    }

    return jitter >> 3; // Aalway pos, divide by 8 for .1uSec precision
}

void Print_JitterInfo(uint8_t taskNUM) {
    if (taskNUM == 1) { // Task 1 data
        printf_lite("Task 1 Jitter Info:\n\r");
        printf_lite("Max Jitter: %d us\n\r", MaxJitter_Task1);
        printf_lite("Jitter Size: %d\n\r", JITTERSIZE_TASK1);
        printf_lite("Jitter Histogram:\n\r");

        for (uint32_t i = 0; i < JITTERSIZE_TASK1; i++) {
            if (JitterHistogram_Task1[i] > 0) {
                printf_lite("Jitter %d us: %d occurrences\n\r", i, JitterHistogram_Task1[i]);
            }
        }
    } 
    else if (taskNUM == 2) { // Task 2 data
        printf_lite("Task 2 Jitter Info:\n\r");
        printf_lite("Max Jitter: %d us\n\r", MaxJitter_Task2);
        printf_lite("Jitter Size: %d\n\r", JITTERSIZE_TASK2);
        printf_lite("Jitter Histogram:\n\r");

        for (uint32_t i = 0; i < JITTERSIZE_TASK2; i++) {
            if (JitterHistogram_Task2[i] > 0) {
                printf_lite("Jitter %d us: %d occurrences\n\r", i, JitterHistogram_Task2[i]);
            }
        }
    } 
    else {
        printf_lite("Stop fucking with print jitter info\n\r");
    }
}

int32_t GetPeriodicTask1Jitter(void){
    return MaxJitter_Task1;
}

int32_t GetPeriodicTask2Jitter(void){
    return MaxJitter_Task2;
}

#endif /* ifdef 0w */

void OS_PeriodicTask1(void){
    User_PeriodicTask1();
    #ifdef CALCULATE_TASK_JITTER 
        static uint32_t lastTime = 0;
        uint32_t thisTime = OS_Time();

        if (lastTime != 0) {  // Avoid incorrect large jitter on first call
            uint32_t jitter = ComputeJitter(lastTime, thisTime, UserTask1_Period);
            MaxJitter_Task1 = (MaxJitter_Task1 > jitter) ? MaxJitter_Task1 : jitter; 
        if (jitter < JITTERSIZE_TASK1) {
            JitterHistogram_Task1[jitter]++;
        }
    }

    lastTime = thisTime;  // Update lastTime for next execution
#endif
}

void OS_PeriodicTask2(void){
    User_PeriodicTask2();

    #ifdef CALCULATE_TASK_JITTER 
        static uint32_t lastTime = 0;  // Persist across function calls
        uint32_t thisTime = OS_Time(); // Get the current time

        if (lastTime != 0) {  // Skip first execution to avoid incorrect jitter
            uint32_t jitter = ComputeJitter(lastTime, thisTime, UserTask2_Period);
            MaxJitter_Task2 = (MaxJitter_Task2 > jitter) ? MaxJitter_Task2 : jitter; 

            if (jitter < JITTERSIZE_TASK2) {  // Prevent array out-of-bounds
                JitterHistogram_Task2[jitter]++;
            }
        }

        lastTime = thisTime;  // Update lastTime for next execution
    #endif /* CALCULATE_TASK_JITTER */
}

int OS_AddPeriodicThread(void (*task)(void), uint32_t period, uint32_t priority) {
    int status = StartCritical();  // Enter critical section

    if (periodicThreadCount == 0) {
        User_PeriodicTask1 = task;
        UserTask1_Period = period;
        Timer3A_Init(OS_PeriodicTask1, period, priority);
        periodicThreadCount++;  // Increment thread count
    } 
    else if (periodicThreadCount == 1) {
        User_PeriodicTask2 = task;
        UserTask2_Period = period;
        Timer4A_Init(OS_PeriodicTask2, period, priority);
        periodicThreadCount++;  // Increment thread count
    } 
    else {
        //error checing
    }

    EndCritical(status);  // Exit critical section
    return 1;  // Success
}

//int OS_AddProcess(void(*entry)(void), void *text, void *data, unsigned long stackSize, unsigned long priority){
//    //load prog using elf loader into heap, deal with base reg and function fills
//    //allocate PCB and populate it
//    //init a thread at the program entry point
//    //let the hoe run
//    //get a pcb
//    int status = StartCritical();
//
//    PD1 ^= 0x02;
//    if(totalProcessCount > NUMPROCESSES || totalThreadCount > NUMTHREADS){
//        Free(text);
//        Free(data);
//        EndCritical(status);
//        return OS_FAIL;
//    }
//
//    PCB_t* pcb  = GetOpenPCB();
//
//    //PD2 ^= 0x04;
//    //PD1 ^= 0x02;
//    //PD1 ^= 0x02;
//    //PD1 ^= 0x02;
//    if(pcb == NULL){
//        Free(text);
//        Free(data);
//        EndCritical(status);
//        return OS_FAIL;
//    }
//
//    pcb->PID = nextPID;
//    pcb->codeSection = text;
//    pcb->dataSection = data;
//    pcb->threadCount = 1;// assuming a main thread 
//        
//    TCB_t* openTCB_ptr = GetOpenTCB(entry, stackSize, PROCESS_CALLER, pcb);
//    if(openTCB_ptr == NULL){
//        Free(text);
//        Free(data);
//        EndCritical(status);
//        return OS_FAIL;
//    }
//
//    //initialize tcb params for a active thread
//    openTCB_ptr->priority = priority;
//    openTCB_ptr->timeToWakeUpMS = 0;
//    openTCB_ptr->id = nextThreadID;
//    openTCB_ptr->parentProcess = pcb;
//
//    OS_AddTCBToActivePool(openTCB_ptr);
//
//    totalThreadCount++;
//    totalProcessCount++;
//    nextThreadID++;
//    nextPID++;
//    //PD2 ^= 0x04;
//    printf("added Process success\n\r");
//    //OS_Sleep(500);
//    //PD1 ^= 0x02;
//    //PD1 ^= 0x02;
//    //PD1 ^= 0x02;
//    PD1 ^= 0x02;
//    EndCritical(status);
//    return OS_SUCCESS;
//}

//int OS_KillProcess(PCB_t* pcb) {
//    // Free code section
//    if (Free(pcb->codeSection) == WAIT_REJECTED) {
//        if (OS_Fifo_Put((uint32_t)pcb->codeSection, OS_FreePtrs_FIFO) == 0) {
//            printf("Free FIFO Put Failed (codeSection)\n\r");
//        }
//    }
//
//    // Free data section
//    if (Free(pcb->dataSection) == WAIT_REJECTED) {
//        if (OS_Fifo_Put((uint32_t)pcb->dataSection, OS_FreePtrs_FIFO) == 0) {
//            printf("Free FIFO Put Failed (dataSection)\n\r");
//        }
//    }
//
//    // Free PCB itself
//    if (Free(pcb) == WAIT_REJECTED) {
//        if (OS_Fifo_Put((uint32_t)pcb, OS_FreePtrs_FIFO) == 0) {
//            printf("Free FIFO Put Failed (pcb)\n\r");
//        }
//    }
//    return 0;
//}



int OS_Get_PeriodicTaskThread_Jitter(void){
    return 42;
}

uint32_t OS_Id(void){
    PD1 ^= 0x02;
    PD1 ^= 0x02;
    return RunPt->id;
}

void PeriodicTask_IncrementGlobalTimeMS(void){
    //GPIO_PORTD_DATA_R ^= 0x08;
  globalTimeMS++;
}

void OS_Time_Init(void){
    Timer1_Init_Global_Clock_Count();
}

uint32_t OS_Time(void){
  return TIMER1_TAR_R;
}

uint32_t OS_TimeDifference(uint32_t start, uint32_t stop){
    uint32_t diff;
    if(start > stop){
        diff = (UINT32_MAX - stop) + start;
    }else{
        diff = stop - start;
    }
  return diff;
}

void OS_ClearMsTime(void){    
  globalTimeMS = 0;
}

uint32_t OS_MsTime(void){
  return globalTimeMS;
}

void Timer1_Init_Global_Clock_Count(void){
  volatile uint32_t delay;
  SYSCTL_RCGCTIMER_R    |= 0x02;                // 0) activate TIMER1a
  delay++; delay++;                            // allow time to finish activating
  TIMER1_CTL_R           = 0x00000000;          // 1) disable TIMER1A during setup
  TIMER1_CFG_R           = 0x00000000;          // 2) configure for 32-bit mode
  TIMER1_TAMR_R          = 0x00000012;          // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R         = 0xFFFFFFFF;          // 4) reload value
  TIMER1_TAPR_R          = 0;                   // 5) bus clock resolution
  TIMER1_CTL_R           = 0x00000001;          // 10) enable TIMER1A
}

int StreamToDevice=0;

//int fputc (int ch, FILE *f) { 
//  if(StreamToDevice==1){
//    if(eFile_Write(ch)){
//       OS_EndRedirectToFile();
//       return 1;
//    }
//    return 0;
//  }
//  UART_OutChar(ch);
//  return ch;
//}
//
//int fgetc (FILE *f){
//  char ch = UART_InChar();
//  UART_OutChar(ch);
//  return ch;
//}

//int OS_RedirectToFile(const char *name){
//    if(eFile_Create(name)){
//        return 1;
//    }
//    if(eFile_WOpen(name)){
//        return 1;
//    }
//    int status = StartCritical();
//    SerialOutPtr = eFile_Write;
//    EndCritical(status);
//    return 0;
//
//}

//int OS_EndRedirectToFile(void){
//    if(eFile_WClose()){
//        return 1;
//    }
//    int status = StartCritical();
//    SerialOutPtr = UART_OutChar;
//    EndCritical(status);
//    return 0;
//}

int OS_RedirectToUART(void){
  StreamToDevice = 0;
  return 0;
}

int OS_RedirectToST7735(void){
  return 1;
}

void _putchar(char character){
    SerialOutPtr(character);
}

void *npfputc(char c, void *ctx){
    _putchar(c);
    return 0;
}

void StartupDelay(void){
  for(uint16_t i = 0; i < UINT16_MAX; i++){}
}