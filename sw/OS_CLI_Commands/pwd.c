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
CMDS_RESULT CLI_pwd(void) {
    CLI_Wait();  // Block other CLI actions

    TCHAR* path = (TCHAR*) Malloc(sizeof(char) * (_MAX_LFN + 1));
    if (path == NULL) {
        printf("CLI_pwd ERROR: Malloc failed!\n");
        CLI_Signal();
        return PWD_FAIL;
    }

    FRESULT res = f_getcwd(path, _MAX_LFN + 1);
    if (res != FR_OK) {
        printf("CLI_pwd ERROR: f_getcwd failed with error code %d\n", res);
        Free(path);
        CLI_Signal();
        return PWD_FAIL;  
    }

    printf("%s\n", path);  

    Free(path);

    CLI_Signal();
    return CMDS_SUCCESS;  // Success
}



