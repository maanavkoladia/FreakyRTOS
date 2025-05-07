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
CMDS_RESULT CLI_cat(char* fileName) {
    CLI_Wait();
    
    // Open the file for reading
    if (eFile_ROpen(fileName)) {
        printf("Error Opening File\n\r");
        CLI_Signal();
        return CAT_FAIL;  // Failed to open
    }
    
    char byte;

    // Read each byte and print it as a char
    while (!eFile_ReadNext(&byte)) {
        printf("%c", byte);  // Print character, not decimal value
    }

    printf("\n\n");

    // Close the file after reading
    if (eFile_RClose()) {
        printf("Error Closing File\n\r");
        CLI_Signal();
        return CAT_FAIL;  // Failed to close
    }

    CLI_Signal();
    return CMDS_SUCCESS;
}
