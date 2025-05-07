/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "CLI_Commands.h"
#include "../OS.h"
#include "../OS_FatFS/ff.h"
#include "printf.h"
/* ================================================== */
/*            GLOBAL VARIABLE DECLARATIONS            */
/* ================================================== */
/* ================================================== */
/*                 FUNCTION PROTOTYPES                */
/* ================================================== */
/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
CMDS_RESULT CLI_cd(char* name){
    CLI_Wait();
    if(eFile_DOpen(name)){
        printf("error opening Dir\n\r"); 
        CLI_Signal();
        return CD_FAIL;
    }
    CLI_Signal();
    return CMDS_SUCCESS;
}


