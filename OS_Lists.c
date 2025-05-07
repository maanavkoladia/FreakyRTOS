/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "OS_Lists.h"
#include "OS.h"

#include <stddef.h>
#include <stdint.h>


/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
TCB_t* ActivePriorityList[PRIORITY_RANGE];

//for finding higestest priority fast with asm intruction
uint32_t activeIdx_ActivePriorityList = 0;
//this should be a singly LL, no point in doubly
TCB_t* sleepingPoolHead;

uint8_t activeThreadCount;
uint8_t sleepingThreadCount;

TCB_t TCB_List[NUMTHREADS];

uint32_t Stacks[NUMTHREADS][STACKSIZE];
PCB_t PCBPool[NUMPROCESSES];
extern TCB_t* RunPt;
extern TCB_t* nextPtCS;
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */
extern void ContextSwitch(void);

extern void PeriodicTask_IncrementGlobalTimeMS(void);

extern int FirstOpenPriorityList(uint32_t leading0s);
/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

void Scheduler(void){

    int status = StartCritical();
    
    //Find the highest priority list that has threads
//    for(highestPri_idx = 0; highestPri_idx < PRIORITY_RANGE; highestPri_idx++){
//        if(ActivePriorityList[highestPri_idx] != NULL){
//            break;
//        }
//    }
    
    //int highestPri_idx = FirstOpenPriorityList(activeIdx_ActivePriorityList); 
    int highestPri_idx = __builtin_ctz(activeIdx_ActivePriorityList);

    nextPtCS = ActivePriorityList[highestPri_idx];

    ActivePriorityList[highestPri_idx] = ActivePriorityList[highestPri_idx]->nextTCBptr;

    EndCritical(status);
}

void OS_ListsInit(void){
    activeThreadCount = 0;
    sleepingThreadCount = 0;

    activeIdx_ActivePriorityList = 0;

    sleepingPoolHead = NULL;

    for(int i = 0; i < NUMTHREADS; i++){
        TCB_List[i].isDead = true;
        TCB_List[i].nextTCBptr = NULL;
        TCB_List[i].prevTCBPtr = NULL;
        TCB_List[i].stackPtr = NULL;
        TCB_List[i].timeToWakeUpMS = 0;
    }
    
    for(int i = 0; i < PRIORITY_RANGE; i++){
        ActivePriorityList[i] = NULL;
    }

    for(int i = 0; i < NUMPROCESSES; i++){
            PCBPool[i].PID = -1;
            PCBPool[i].codeSection = NULL;
            PCBPool[i].dataSection= NULL;
            PCBPool[i].threadCount = 0;
    }
}

void OS_AddTCBToActivePool(TCB_t* tcb) {
    if (tcb == NULL) return; // Prevent NULL pointer access
    if (tcb->priority >= PRIORITY_RANGE) return; // Invalid priority, exit

    // Ensure TCB has clean pointers before adding
    tcb->nextTCBptr = NULL;
    tcb->prevTCBPtr = NULL;

    if (ActivePriorityList[tcb->priority] == NULL) {
        // First thread in this priority level
        ActivePriorityList[tcb->priority] = tcb;
        activeIdx_ActivePriorityList |= (1 << tcb->priority);
        tcb->nextTCBptr = tcb;  // Circular linked list: next points to itself
        tcb->prevTCBPtr = tcb;  // Circular linked list: prev points to itself
    } else {
        // Get the current last node (tail)
        TCB_t* tail = ActivePriorityList[tcb->priority]->prevTCBPtr;

        // Insert new TCB between tail and head
        tail->nextTCBptr = tcb;
        tcb->prevTCBPtr = tail;
        tcb->nextTCBptr = ActivePriorityList[tcb->priority];

        // Update head's prev pointer to point to new TCB (new tail)
        ActivePriorityList[tcb->priority]->prevTCBPtr = tcb;
    }

    activeThreadCount++; // Increment only if TCB was successfully added
}
void OS_RemoveTCBFromActivePool(TCB_t* tcb){
    if (ActivePriorityList[tcb->priority] == tcb && tcb->nextTCBptr == tcb) { 
        // Only one TCB in the list, remove it
        ActivePriorityList[tcb->priority] = NULL;
        activeIdx_ActivePriorityList &= ~(1 << tcb->priority);
    } else {
        TCB_t* currPrev = tcb->prevTCBPtr;
        TCB_t* currNext = tcb->nextTCBptr;

        // If removing head, update it
        if (tcb == ActivePriorityList[tcb->priority]) {
            ActivePriorityList[tcb->priority] = currNext;
        }

        // Update adjacent nodes
        if (currPrev) {
            currPrev->nextTCBptr = currNext;
        }
        if (currNext) {
            currNext->prevTCBPtr = currPrev;
        }
    }
    activeThreadCount--;
}


//deal with wrap around problems
void PeriodicTask_CheckSleepPool(void) {
    PeriodicTask_IncrementGlobalTimeMS();
 
    if(sleepingThreadCount == 0){return;}

    int8_t threadremoved = 0;
    int32_t status = StartCritical();
;
    while (sleepingThreadCount > 0 && 
        ((int32_t)(OS_MsTime() - sleepingPoolHead->timeToWakeUpMS) >= 0)) 
    {
        TCB_t* wakeMeUp = sleepingPoolHead;
        // Remove the head of the sleep pool.
        OS_RemoveTCBFromSleepPool();
        // Add the thread back to the active pool.
        OS_AddTCBToActivePool(wakeMeUp);
        
        threadremoved++;
    }

    if(threadremoved >0){
        Scheduler();
        ContextSwitch();
        EndCritical(status);
    }else {
        EndCritical(status);
    }
}

//should add to sleep pool based on how close OS_MsTime is wake up time, deal wrap around problems
// When inserting, we assume that the tcb->timeToWakeUpMS is already set.
void OS_AddTCBToSleepPool(TCB_t* tcb) {
    
    //assuming sleephead is null
    if (sleepingPoolHead == NULL) {
        sleepingPoolHead = tcb;
        tcb->nextTCBptr = NULL;
        tcb->prevTCBPtr = NULL;
    } 
    
    //if head is later than the tcb im trying to insert
    else if ((int32_t)(tcb->timeToWakeUpMS - sleepingPoolHead->timeToWakeUpMS) < 0) {
        // Insert before the current head.
        tcb->nextTCBptr = sleepingPoolHead;
        tcb->prevTCBPtr = NULL;
        sleepingPoolHead->prevTCBPtr = tcb;
        sleepingPoolHead = tcb;
    } 
    
    else {
        TCB_t* current = sleepingPoolHead;
        // Traverse the list until we find a TCB whose wake-up time is later than tcb's.
        while (current->nextTCBptr != NULL && ((int32_t)(tcb->timeToWakeUpMS - current->nextTCBptr->timeToWakeUpMS) >= 0)) {
            current = current->nextTCBptr;
        }
        // Insert tcb after 'current'
        tcb->nextTCBptr = current->nextTCBptr;
        tcb->prevTCBPtr = current;
        
        // If we're not inserting at the very end, update the next node's previous pointer.
        if (current->nextTCBptr != NULL) {
            current->nextTCBptr->prevTCBPtr = tcb;
        }
        current->nextTCBptr = tcb;
    }    
    
    sleepingThreadCount++;
}
//should add thread back to active pool and removing it from sleep pool
void OS_RemoveTCBFromSleepPool(void) {
    if(sleepingPoolHead == NULL){
        return;
    }
    if(sleepingThreadCount == 1){
        sleepingPoolHead = NULL;
    }else{
        sleepingPoolHead = sleepingPoolHead->nextTCBptr;
    }
    sleepingThreadCount--;
}

//assumming 0 is the highest pri and pri_range-1 is lowest
//using a circluar doubly LL st the pointer at the priority index is the oldest in that pripority
void OS_AddTCBToBlockedList(sema4_t* sema4, TCB_t* blockMePweeese) {
    if (blockMePweeese == NULL || sema4 == NULL) { return; }
    
    if (sema4->blockList[blockMePweeese->priority] == NULL) { // If list is empty
        sema4->blockList[blockMePweeese->priority] = blockMePweeese;
        blockMePweeese->nextTCBptr = blockMePweeese; // Circular linking to itself
        blockMePweeese->prevTCBPtr = blockMePweeese; // Circular linking to itself
        sema4->activeListsidx_BlockingList |= (1 << blockMePweeese->priority);
    } else {
        TCB_t* head = sema4->blockList[blockMePweeese->priority];
        TCB_t* tail = head->prevTCBPtr; // Get the last element in the circular list

        // Insert at the end
        tail->nextTCBptr = blockMePweeese;
        blockMePweeese->prevTCBPtr = tail;
        blockMePweeese->nextTCBptr = head;
        head->prevTCBPtr = blockMePweeese;
    }
}

TCB_t* OS_RemoveTCBFromBlockedList(sema4_t* sema4) {
    if (sema4 == NULL) {
        return NULL;
    }

    // Get the highest priority index efficiently using bitfield
    //uint32_t highestPri_idx = FirstOpenPriorityList(sema4->activeListsidx_BlockingList);
    
    int highestPri_idx = __builtin_ctz(sema4->activeListsidx_BlockingList);

    // If no priority found, return NULL
    if (highestPri_idx >= PRIORITY_RANGE || sema4->blockList[highestPri_idx] == NULL) {
        return NULL;
    }

    TCB_t* wakeUp = sema4->blockList[highestPri_idx];

    // Case: Only one node in the list
    if (wakeUp->nextTCBptr == wakeUp) {
        sema4->blockList[highestPri_idx] = NULL; // List becomes empty
        sema4->activeListsidx_BlockingList &= ~(1 << highestPri_idx); // Update bitmask
    } else {
        // Remove wakeUp from the circular list
        TCB_t* prev = wakeUp->prevTCBPtr;
        TCB_t* next = wakeUp->nextTCBptr;

        prev->nextTCBptr = next;
        next->prevTCBPtr = prev;

        // Update head of the list
        sema4->blockList[highestPri_idx] = next;
    }

    // Clear pointers before returning
    wakeUp->nextTCBptr = NULL;
    wakeUp->prevTCBPtr = NULL;

    return wakeUp;
}

//void InitStack_ForThread(uint8_t stack_idx, void(*task)(void)) {
//    TCB_List[stack_idx].stackPtr = &Stacks[stack_idx][STACKSIZE-16];
//    Stacks[stack_idx][STACKSIZE-1] = 0x01000000; // Thumb bit 
//    Stacks[stack_idx][STACKSIZE-2] = (int32_t)(task);
//    Stacks[stack_idx][STACKSIZE-3] = 0x14141414; // R14//
//    Stacks[stack_idx][STACKSIZE-4] = 0x12121212; // R12
//    Stacks[stack_idx][STACKSIZE-5] = 0x03030303; // R3
//    Stacks[stack_idx][STACKSIZE-6] = 0x02020202; // R2
//    Stacks[stack_idx][STACKSIZE-7] = 0x01010101; // R1
//    Stacks[stack_idx][STACKSIZE-8] = 0x00000000; // R0
//    Stacks[stack_idx][STACKSIZE-9] = 0x11111111; // R11
//    Stacks[stack_idx][STACKSIZE-10] = 0x10101010; // R10
//    Stacks[stack_idx][STACKSIZE-11] = 0x09090909; // R9
//    Stacks[stack_idx][STACKSIZE-12] = 0x08080808; // R8
//    Stacks[stack_idx][STACKSIZE-13] = 0x07070707; // R7
//    Stacks[stack_idx][STACKSIZE-14] = 0x06060606; // R6
//    Stacks[stack_idx][STACKSIZE-15] = 0x05050505; // R5
//    Stacks[stack_idx][STACKSIZE-16] = 0x04040404; // R4
//}

void InitStack_ForThread(uint32_t* sp, size_t stackSize, TCB_t* newTCB, void (*task)(void)) {
    newTCB->stackPtr = &sp[stackSize-16];
    sp[stackSize-1] = 0x01000000; // Thumb bit 
    sp[stackSize-2] = (uint32_t)(task);
    sp[stackSize-3] = 0x14141414; // R14//
    sp[stackSize-4] = 0x12121212; // R12
    sp[stackSize-5] = 0x03030303; // R3
    sp[stackSize-6] = 0x02020202; // R2
    sp[stackSize-7] = 0x01010101; // R1
    sp[stackSize-8] = 0x00000000; // R0
    sp[stackSize-9] = 0x11111111; // R11
    sp[stackSize-10] = 0x10101010; // R10
    sp[stackSize-11] = 0x09090909; // R9
    sp[stackSize-12] = 0x08080808; // R8
    sp[stackSize-13] = 0x07070707; // R7
    sp[stackSize-14] = 0x06060606; // R6
    sp[stackSize-15] = 0x05050505; // R5
    sp[stackSize-16] = 0x04040404; // R4
    sp[0] = 0x12345678;
    sp[1] = 0x12345678;
    sp[2] = 0x12345678;
    sp[3] = 0x12345678;
}

void InitStack_ForThread_Process(uint32_t* sp, size_t stackSize, TCB_t* newTCB, void (*task)(void), void* static_baseR) {
    newTCB->stackPtr = &sp[stackSize-16];
    sp[stackSize-1] = 0x01000000; // Thumb bit 
    sp[stackSize-2] = (uint32_t)(task);
    sp[stackSize-3] = 0x14141414; // R14//
    sp[stackSize-4] = 0x12121212; // R12
    sp[stackSize-5] = 0x03030303; // R3
    sp[stackSize-6] = 0x02020202; // R2
    sp[stackSize-7] = 0x01010101; // R1
    sp[stackSize-8] = 0x00000000; // R0
    sp[stackSize-9] = 0x11111111; // R11
    sp[stackSize-10] = 0x10101010; // R10
    sp[stackSize-11] = (uint32_t) static_baseR; // R9
    sp[stackSize-12] = 0x08080808; // R8
    sp[stackSize-13] = 0x07070707; // R7
    sp[stackSize-14] = 0x06060606; // R6
    sp[stackSize-15] = 0x05050505; // R5
    sp[stackSize-16] = 0x04040404; // R4
    sp[0] = 0x12345678;
    sp[1] = 0x12345678;
    sp[2] = 0x12345678;
    sp[3] = 0x12345678;
}

TCB_t* GetOpenTCB(void(*task)(void), size_t stackSize, int caller, PCB_t* pcb){
    int8_t openTCB_idx = -1;
    for(int8_t i = 0; i < NUMTHREADS; i++){
        if(TCB_List[i].isDead){
            openTCB_idx = i;
            break;
        }
    }
    
    //there are no available tcbs
    if(openTCB_idx == -1){
        return NULL;
    }

    TCB_t* newTCB = &TCB_List[openTCB_idx];
    newTCB->isDead = 0;
    uint32_t* stack = Stacks[openTCB_idx];
    //if(caller == PROCESS_CALLER){
    //    InitStack_ForThread_Process(stack, stackSize, newTCB, task, pcb->dataSection);
    //}else{
    //    InitStack_ForThread(stack, stackSize, newTCB, task);
    //}
    
    InitStack_ForThread(stack, stackSize, newTCB, task);

    return newTCB;
}

PCB_t* GetOpenPCB(void){
    for(int i = 0; i < NUMPROCESSES; i++){
        if(PCBPool[i].PID == -1){
            return &PCBPool[i];
        }
    }
    return NULL;
}

//find first highest pri task
TCB_t* GetFirstTask(TCB_t* runptr){
    for(int i = 0; i < PRIORITY_RANGE; i++){
        if(ActivePriorityList[i] != NULL){
            return ActivePriorityList[i];
        }
    }
    return NULL;
}
