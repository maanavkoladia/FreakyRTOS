/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "CLI_Commands.h"
#include "../OS.h"
#include "../OS_FatFS/ff.h"
#include "printf.h"
#include <stddef.h>
/* ================================================== */
/*            GLOBAL VARIABLE DECLARATIONS            */
/* ================================================== */
/* ================================================== */
/*                 FUNCTION PROTOTYPES                */
/* ================================================== */
/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
CMDS_RESULT CLI_ls(void) {
    CLI_Wait();
    
    // Allocate buffer for current path
    char* path = (char*)Malloc(_MAX_LFN + 1);
    if (path == NULL) {
        printf("Error: Malloc failed for path buffer\n");
        CLI_Signal();
        return LS_FAIL;
    }

    // Get the current working directory path
    if (f_getcwd(path, _MAX_LFN) != FR_OK) {
        printf("Error getting PWD in ls\n");
        Free(path);
        CLI_Signal();
        return LS_FAIL;
    }

    // Allocate and open the directory object
    DIR* pwd = (DIR*)Malloc(sizeof(DIR));
    if (pwd == NULL) {
        printf("Error: Malloc failed for DIR\n");
        Free(path);
        CLI_Signal();
        return LS_FAIL;
    }

    if (f_opendir(pwd, path) != FR_OK) {
        printf("Error opening PWD in ls\n");
        Free(path);
        Free(pwd);
        CLI_Signal();
        return LS_FAIL;
    }
    
    // Allocate space for the file info structure
    FILINFO* fi = (FILINFO*)Malloc(sizeof(FILINFO));
    if (fi == NULL) {
        printf("Error: Malloc failed for FILINFO\n");
        f_closedir(pwd);
        Free(path);
        Free(pwd);
        CLI_Signal();
        return LS_FAIL;
    }

    // Read directory entries
    while (1) {
        FRESULT res = f_readdir(pwd, fi);
        if (res != FR_OK) {
            printf("Error reading directory entry: %d\n", res);
            break;
        }

        // If fname[0] is zero, it means the end of directory
        if (fi->fname[0] == 0) {
            break;
        }
        
        if(fi->fattrib == 16){
            printf("d %s %dbytes\n", fi->fname, fi->fsize);
        }else{
            printf(". %s %dbytes\n", fi->fname, fi->fsize);
        }
    }

    // Cleanup
    f_closedir(pwd);
    Free(fi);
    Free(pwd);
    Free(path);

    CLI_Signal();
    return CMDS_SUCCESS;
}
