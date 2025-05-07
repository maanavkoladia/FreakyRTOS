#ifndef OS_LISTS_H 
#define OS_LISTS_H
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "OS.h"
/* ================================================== */
/*            GLOBAL VARIABLE DECLARATIONS            */
/* ================================================== */
#define THREAD_CALLER 0
#define PROCESS_CALLER 1
/* ================================================== */
/*                 FUNCTION PROTOTYPES                */
/* ================================================== */
void Scheduler(void);

void OS_AddTCBToSleepPool(TCB_t* tcb);
void OS_RemoveTCBFromSleepPool(void);

void OS_AddTCBToActivePool(TCB_t* tcb);
void OS_RemoveTCBFromActivePool(TCB_t* tcb);

void OS_AddTCBToBlockedList(sema4_t* sema4, TCB_t* blockMePweeese);
TCB_t* OS_RemoveTCBFromBlockedList(sema4_t* sema4);

void OS_ListsInit(void);


void PeriodicTask_CheckSleepPool(void);

TCB_t* GetOpenTCB(void(*task)(void), size_t stackSize, int caller, PCB_t* pcb);

//find first highest pri task
TCB_t* GetFirstTask(TCB_t* runptr);

PCB_t* GetOpenPCB(void);
#endif 

