/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "CLI_Commands.h"
#include "../ELF_Utils/loader.h"
#include "printf.h"
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
extern const ELFEnv_t ELFEnv;

#define PD0  (*((volatile uint32_t *)0x40007004))
#define PD1  (*((volatile uint32_t *)0x40007008))
#define PD2  (*((volatile uint32_t *)0x40007010))
#define PD3  (*((volatile uint32_t *)0x40007020))
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */


/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
//return 0 if sucessful
//1 if error
CMDS_RESULT CLI_exec(char* fileName){
    CLI_Wait();
    //PD1 ^= 0x02;
    //PD1 ^= 0x02;
    //PD1 ^= 0x02;
    if(exec_elf(fileName, &ELFEnv) != 1){
        printf("Exec Elf failed\n\r");
        CLI_Signal();
        return EXEC_FAIL;
    }

    printf("Elf should be running\n\r");
    //PD1 ^= 0x01;
    //PD1 ^= 0x02;
    //PD1 ^= 0x02;
    CLI_Signal();
    return CMDS_SUCCESS;
}
