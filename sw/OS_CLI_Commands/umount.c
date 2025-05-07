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
CMDS_RESULT CLI_umount(void) {
    CLI_Wait();

    if(eFile_Unmount()){
        printf("Error Unmounting FS\n\r");
        CLI_Signal();
        return MOUNT_FAIL; 
    }

    printf("FS Unmounted Sucessfully\n\r");
    CLI_Signal();
    return CMDS_SUCCESS;
}
