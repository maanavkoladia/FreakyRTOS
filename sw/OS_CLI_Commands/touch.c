/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "CLI_Commands.h"
#include "printf.h"
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */


/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */


/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
//return 0 if sucessful
//1 if error
CMDS_RESULT CLI_touch(char* fileName){
    CLI_Wait();
    //add error checing for if the file name is too long
    if(eFile_Create(fileName)){
        printf("error Creating file\n\r");
        CLI_Signal();
        return TOUCH_FAIL;
    }
    CLI_Signal();
    return CMDS_SUCCESS;
}
