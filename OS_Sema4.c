/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>
#include <stddef.h>
#include "OS_Lists.h"
#include "OS.h"

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
extern void ContextSwitch(void);

extern TCB_t* RunPt;
extern TCB_t* nextPtCS;

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */
#define IXPSR_MASK 0xFF
extern uint32_t GetxPSR(void);

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
void OS_InitSemaphore(sema4_t *semaPt, int32_t value){
    semaPt->Value = value;
    semaPt->activeListsidx_BlockingList = 0;
    for(int i = 0; i < PRIORITY_RANGE; i++){
        semaPt->blockList[i] = NULL;
    }
}

int OS_Wait(sema4_t *semaPt){
    int status = StartCritical();  
    uint32_t ipsr = GetxPSR() & IXPSR_MASK;
    
    if(ipsr != 0 && semaPt->Value < 1){//not in thread mdoe and semaphore not available
        EndCritical(status);
        return WAIT_REJECTED;
    } 
    semaPt->Value--;
    if(semaPt->Value < 0){
        OS_RemoveTCBFromActivePool(RunPt);
        OS_AddTCBToBlockedList(semaPt, RunPt);
        Scheduler();
        ContextSwitch();  // **Force a context switch**
        EndCritical(status);   // Restore interrupts **before blocking**
    } else {
        EndCritical(status);  // If semaphore was available, continue normally
    }
    return OS_SUCCESS;
}

void OS_Signal(sema4_t *semaPt){
    int status = StartCritical();
    semaPt->Value++;
    TCB_t* unblocked = OS_RemoveTCBFromBlockedList(semaPt);
    if(unblocked != NULL){
        OS_AddTCBToActivePool(unblocked);
        Scheduler();
        ContextSwitch();
        EndCritical(status);
    }else{
        EndCritical(status);
    }
}

void OS_bWait(sema4_t *semaPt){
    OS_Wait(semaPt);
}

void OS_bSignal(sema4_t *semaPt){
    OS_Signal(semaPt);
}

//void OS_bWait(sema4_t *semaPt){
//    int status = StartCritical();  
//    if(semaPt->Value == 0){  // If semaphore is 0, block
//        OS_AddTCBToBlockedList(semaPt, RunPt);
//        Scheduler();          // Update scheduling to select next thread
//        ContextSwitch();      // Force context switch to avoid running blocked thread
//        EndCritical(status);  // Restore interrupts **before blocking**
//    } else {
//        semaPt->Value = 0;   // If semaphore was 1, consume it and proceed
//        EndCritical(status);
//    }
//}
//
//void OS_bSignal(sema4_t *semaPt){
//    int status = StartCritical();
//    
//    TCB_t* unblocked = OS_RemoveTCBFromBlockedList(semaPt);
//    if(unblocked != NULL){
//        OS_AddTCBToActivePool(unblocked);
//        Scheduler();         // Ensure correct scheduling order
//        ContextSwitch();     // Force context switch if a thread was waiting
//        EndCritical(status);
//    } else {
//        semaPt->Value = 1;   // If no threads are waiting, reset semaphore to 1
//        EndCritical(status);
//    }
//}
