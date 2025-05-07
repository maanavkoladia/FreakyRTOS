/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "CLI_Commands.h"
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
CMDS_RESULT CLI_mkfs(void) {
    CLI_Wait();

    if(eFile_Format()){
        printf("Error Formating FS for F32\n\r");
        CLI_Signal();
        return MOUNT_FAIL; 
    }

    printf("FS Formatted for F32 Sucessfully\n\r");
    CLI_Signal();
    return CMDS_SUCCESS;
}
