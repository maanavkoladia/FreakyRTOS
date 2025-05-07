/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "CLI_Commands.h"
#include "../OS_FatFS/ff.h"
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
CMDS_RESULT CLI_mkdir(char* dirName){
    CLI_Wait();
    //add error checing for if the file name is too long
    if(f_mkdir(dirName)){
        printf("error Creating dir\n\r");
        CLI_Signal();
        return MKDIR_FAIL;
    }
    CLI_Signal();
    return CMDS_SUCCESS;
}
