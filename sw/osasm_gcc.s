    .syntax unified   // Unified assembly mode
    .cpu cortex-m4    // Ensure the assembler knows the correct CPU
    .thumb            // Enable Thumb mode
    
    .extern RunPt                // Declare external variable
    .extern nextPtCS
    .extern OS_AddTCBToBlockedList
    .extern OS_RemoveTCBFromBlockedList
    .extern SVC_Vector_Table 

    .global StartOS
    .global OSStartHang
    .global ContextSwitch
    .global PendSV_Handler
    .global SVC_Handler
    .global FirstOpenPriorityList
    .global OS_AddMyNum
    .global DisableInterrupts
    .global EnableInterrupts
    .global StartCritical
    .global EndCritical
    .global WaitForInterrupt
    .global GetxPSR
    .global memcpy
//// Equivalents for hardware registers and constants

.equ    NVIC_INT_CTRL,   0xE000ED04  // Interrupt control state register
.equ    NVIC_SYSPRI14,   0xE000ED22  // PendSV priority register (position 14)
.equ    NVIC_SYSPRI15,   0xE000ED23  // Systick priority register (position 15)
.equ    NVIC_LEVEL14,    0xEF        // Systick priority value (second lowest)
.equ    NVIC_LEVEL15,    0xFF        // PendSV priority value (lowest)
.equ    NVIC_PENDSVSET,  0x10000000  // Value to trigger PendSV exception
.equ    MyNum,           10          // Testing this file for .equ directives

//********************************************************************************************************
//                                         START OS FUNCTION
// Starts the OS by loading the first task's stack and context.
//********************************************************************************************************
.thumb_func
StartOS:
    LDR R0, =RunPt         // Load the address of RunPt (pointer to the current running thread) into R0
    LDR R1, [R0]           // Load the value stored at RunPt (the pointer to the current TCB) into R1
    LDR SP, [R1]           // Load the stack pointer from the current TCB into the processor's SP register (SP = RunPt->sp)
    POP {R4-R11}           // Restore registers R4-R11 from the stack
    POP {R0-R3}            // Restore registers R0-R3 from the stack
    POP {R12}              // Restore register R12 from the stack
    ADD SP, SP, #4         // Adjust stack pointer to discard the saved LR (Link Register) from the initial stack
    POP {LR}               // Load the actual LR (return address) from the stack
    ADD SP, SP, #4         // Adjust stack pointer to discard the saved PSR (Program Status Register)
    CPSIE I                // Enable interrupts at the processor level
    BX LR                  // Branch to the address in LR to start executing the first thread

//********************************************************************************************************
//                               INFINITE OS HANG FUNCTION
// This function is used when the OS halts execution indefinitely.
//********************************************************************************************************
.thumb_func
OSStartHang:
    CPSID I              // Disable interrupts to kill everything
    B       OSStartHang          // Infinite loop (should never get here)

//********************************************************************************************************
//                               PERFORM A CONTEXT SWITCH (From task level)
//                                           void ContextSwitch(void)
//
// ContextSwitch() is called when OS wants to perform a task context switch.
// This function triggers the PendSV exception, which is where the real work is done.
//********************************************************************************************************
.thumb_func
ContextSwitch:
    LDR R0, =NVIC_INT_CTRL
    LDR R1, =NVIC_PENDSVSET
    STR R1, [R0]  
    BX  LR

//********************************************************************************************************
//                                         HANDLE PendSV EXCEPTION
//                                     void OS_CPU_PendSVHandler(void)
//
// PendSV is used to cause a context switch. This is a recommended method for performing
// context switches with Cortex-M. This is because the Cortex-M3 auto-saves half of the
// processor context on any exception and restores the same on return from exception.
//********************************************************************************************************
.thumb_func
PendSV_Handler:
    CPSID I              // Disable interrupts to prevent race conditions during context switching
    PUSH {R4-R11}        // Save the registers R4-R11 (callee-saved registers) onto the stack

    LDR R0, =RunPt       // Load the address of RunPt (pointer to the current running task)
    LDR R1, [R0]         // Load RunPt (current running task's control block)
    STR SP, [R1]         // Save the current task's stack pointer to its control block

    LDR R1, =nextPtCS    // Load the address of the next TCB
    LDR R1, [R1]         // load next TCB   
    STR R1, [R0]         // Update RunPt to point to the next task (switching context)
    LDR SP, [R1]         // Load the next task's stack pointer

    POP {R4-R11}         // Restore registers R4-R11 from the new task's stack
    CPSIE I              // Re-enable interrupts
    BX LR                // Return from exception (context switch complete)

//********************************************************************************************************
//                                         HANDLE SVC EXCEPTION
//                                     void OS_CPU_SVCHandler(void)
//
// SVC is a software-triggered exception to make OS kernel calls from user land. 
// The function ID to call is encoded in the instruction itself, and the location
// can be found relative to the return address saved on the stack on exception entry.
//********************************************************************************************************
//generate an array of OS function ptrs to index at
//load svc based on PC that is pushed on stack
//index access into the array baesd on call number
//push other paramters in USP onto ISP for call with more than 4 paramters
//branch to routine
//come back clean up
//bxlr ,ur done shitter
//r0, r1, r2, r3, lr, sp, pc, xpsr
//TODO: need to add support for funcs w more than 4 inputs, will add laster if needed
//.thumb_func
//SVC_Handler:
//    // R12 is a temp register, use it for the SVC number extraction
//    LDR   R12, [SP, #24]           // Get stacked PC
//    LDRH  R12, [R12, #-2]          // Load SVC instruction halfword
//    ANDS  R12, R12, #0xFF          // Extract SVC number
//    PUSH  {R11}                 // Save callee-saved registers, optional but good practice
//    LDR   R11, =SVC_Vector_Table   // Load address of vector table
//    LDR   R11, [R11, R12, LSL #2]  // Get handler from table
//    PUSH {LR}
//    BLX   R11                      // Call SVC handler, return value in R0
//    POP  {LR}
//    POP   {R11}                 // Restore saved registers
//    STR   R0, [SP]                 // Optionally overwrite stacked R0 to change the return value
//    BX    LR                       // Return from interrupt using EXC_RETURN in LR

//********************************************************************************************************
//us being cool and implenting counting sema4s with blocking using MUTEX LD and STRs
//how counting works, init with 1, val = 0 means 1 has sema4 but no one is waiting
//and val < 0 means how many are blocked waiting for it
//void OS_Wait(sema4_t *semaPt); 
//make a C call to add block list func
//we also need to elevate thread who takes sema4 pri to match the one that is wait if its higher,
//if we want to sweat, this will help to ensure that highests pris arent starved 
//
//void OS_Signal(sema4_t *semaPt); 
//make a C call to remove from blocked list,
//we also need to return the thread back to normal pri once its done with sema4
//
//********************************************************************************************************
    //
//.thumb_func
//OS_Wait:
//    MOV R2 R0 //save ptr to sema4
//    B StartCritical //R0 will now have psr status
//
//    LDREX R1, [R2] //load counter into R2, its the first val
//    SUBS R1, R1, #1 //decrement counter
//    CMP R1, #0
//    BLT OS_AddTCBToBlockedList
//
//
//                        //if counter is now zero, meaning this thread took the sema4, return
//                    //if not add it to the blocked list, scedule the next task, and force a conext switch
//
//
//.thumb_func
//OS_Signal:
//********************************************************************************************************
//                                 ADD CONSTANT VALUE TO INPUT FUNCTION
// extern int OS_AddMyNum(int in1);
// This function takes an input integer and adds the predefined value 'MyNum'.
//********************************************************************************************************
.thumb_func
OS_AddMyNum:
    LDR R1, =MyNum
    ADD R0, R0, R1
    BX LR

//********************************************************************************************************
//                                 FIND NUMBER OF LEADING ZEROS RALTIVE TO A LIST SIZE
//int FirstOpenPriorityList(uint32_t leading0s);
//shoud generatea an index to access at
//********************************************************************************************************
.thumb_func
FirstOpenPriorityList:
    RBIT R0, R0 // reverse the int because pri 0 is highest
    CLZ R0, R0 //get the numbe rof leading 0s
    BX LR
//*********** DisableInterrupts ***************
// disable interrupts
// inputs:  none
// outputs: none
.thumb_func
DisableInterrupts:
        CPSID  I
        BX     LR

//*********** EnableInterrupts ***************
// disable interrupts
// inputs:  none
// outputs: none
.thumb_func
EnableInterrupts:
        CPSIE  I
        BX     LR

//*********** StartCritical ************************
// make a copy of previous I bit, disable interrupts
// inputs:  none
// outputs: previous I bit
.thumb_func
StartCritical:
        MRS    R0, PRIMASK  // save old status
        CPSID  I            // mask all (except faults)
        BX     LR

//*********** EndCritical ************************
// using the copy of previous I bit, restore I bit to previous value
// inputs:  previous I bit
// outputs: none
.thumb_func
EndCritical:
        MSR    PRIMASK, R0
        BX     LR

//*********** WaitForInterrupt ************************
// go to low power mode while waiting for the next interrupt
// inputs:  none
/// outputs: none
.thumb_func
WaitForInterrupt:
        WFI
        BX     LR

.thumb_func
GetxPSR:
    MRS R0, xPSR     // Move the value of xPSR into R0
    BX LR            // Return to caller

.thumb_func
memcpy:
    push    {r4, r5, lr}

    // Check if there's nothing to copy
    cmp     r2, #0
    beq     done

loop:
    cmp     r2, #4
    blt     byte_copy

    // Copy word (4 bytes) at a time
    ldr     r3, [r1], #4
    str     r3, [r0], #4
    subs    r2, r2, #4
    b       loop

byte_copy:
    cmp     r2, #0
    beq     done

    ldrb    r3, [r1], #1
    strb    r3, [r0], #1
    subs    r2, r2, #1
    b       byte_copy

done:
    pop     {r4, r5, pc}
