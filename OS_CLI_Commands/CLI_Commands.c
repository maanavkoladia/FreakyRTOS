/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "CLI_Commands.h"
#include "../OS.h"
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */


/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */
sema4_t cli_mutex;

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
void CLI_Commands_Init(void){
    OS_InitSemaphore(&cli_mutex, 1);
}

void CLI_Wait(void){
    OS_Wait(&cli_mutex);
}

void CLI_Signal(void){
    OS_Signal(&cli_mutex);
}
