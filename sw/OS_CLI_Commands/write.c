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
CMDS_RESULT CLI_write(char* fileName, char* data){
    CLI_Wait();
    if(eFile_WOpen(fileName)){
        printf("Error opening File to Write\n\r");
        CLI_Signal();

        return F_WRITE_FAIL;
    }

    while(*data){
        if(eFile_Write(*data) != 0){
            printf("Error writing file.\n\r");
            eFile_WClose();
            CLI_Signal();
            return F_WRITE_FAIL;
        }
        data++;
    }
    
    if(eFile_WClose()){
        printf("Error CLosing File to Write\n\r");
        CLI_Signal();
        return F_WRITE_FAIL;
    }

    printf("Successfully Wrote to file\n");
    CLI_Signal();
    return CMDS_SUCCESS;
}
