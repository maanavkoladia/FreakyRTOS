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
CMDS_RESULT CLI_mount(void) {
    CLI_Wait();

    if(eFile_Mount()){
        printf("Error Mounting FS\n\r");
        CLI_Signal();
        return MOUNT_FAIL; 
    }

    if(eFile_DOpen("")){
        printf("Error Opening Root on Mount");
        CLI_Signal();
        return MOUNT_FAIL;
    }

    printf("FS Mounted Sucessfully\n\r");
    CLI_Signal();
    return CMDS_SUCCESS;
}
