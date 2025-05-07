#ifndef CLI_COMMANDS_H
#define CLI_COMMANDS_H

/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
//#ifdef FAT32  
#include "../OS_FatFS/eFile.h"
//#endif 

//#ifdef FAT32  
//#include "../OS_CoreFS/eFile.h"
//#endif 
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
typedef enum{
    CMDS_SUCCESS,
    LS_FAIL,
    CAT_FAIL,
    TOUCH_FAIL,
    RM_FAIL,
    F_WRITE_FAIL,
    PWD_FAIL,
    CD_FAIL,
    MOUNT_FAIL,
    UMOUNT_FAIL,
    MKFS_FAIL,
    MKDIR_FAIL,
    EXEC_FAIL
}CMDS_RESULT;

/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */
void CLI_Commands_Init(void);

void CLI_Wait(void);

void CLI_Signal(void);

CMDS_RESULT CLI_mount(void);

CMDS_RESULT CLI_umount(void);

CMDS_RESULT CLI_mkfs(void);

CMDS_RESULT CLI_ls(void);

CMDS_RESULT CLI_cat(char* fileName);

CMDS_RESULT CLI_touch(char* fileName);

CMDS_RESULT CLI_rm(char* fileName);

CMDS_RESULT CLI_pwd(void);

CMDS_RESULT CLI_write(char* fileName, char* data);

CMDS_RESULT CLI_cd(char* name);

CMDS_RESULT CLI_mkdir(char* fileName);

CMDS_RESULT CLI_exec(char* fileName);

#endif  
