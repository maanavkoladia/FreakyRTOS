/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>
#include "OS.h"
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
uint32_t mailbox;
sema4_t boxFree;
sema4_t dataValid;

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */


/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
void OS_MailBox_Init(void){
	mailbox = 0;
    OS_InitSemaphore(&boxFree, 1);
    OS_InitSemaphore(&dataValid, 0);
}

void OS_MailBox_Send(uint32_t data){
	OS_Wait(&boxFree);
	mailbox = data;
	OS_Signal(&dataValid);
}

uint32_t OS_MailBox_Recv(void){
	uint32_t data = 0;
	OS_Wait(&dataValid);
	data = mailbox; // is this not a race condition?
	OS_Signal(&boxFree);
  return data;
}

