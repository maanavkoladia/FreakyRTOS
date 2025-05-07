/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>

#include "../inc/tm4c123gh6pm.h"
#include "../lib/std/string_lite/string_lite.h"
#include "printf.h"

#include "OS.h"
#include "ST7735.h"
#include "UART0int.h"
#include "FIFOsimple.h"

#include "OS_CLI_Commands/CLI_Commands.h"
/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
#define MAX_CMD_LENGTH 64
#define MAX_ARG_LENGTH 20

typedef struct {
  char cmd[MAX_ARG_LENGTH];
  char arg1[MAX_ARG_LENGTH];
  char arg2[MAX_ARG_LENGTH];
} command_t;

char inChar;
char cmdStrBuf[MAX_CMD_LENGTH];
uint16_t cmdStrBuf_idx = 0;

command_t cmdStruct;
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
void ParseCMD(const char *input, command_t *cmdStruct) {
    // Initialize the structure fields to empty strings
    for (int i = 0; i < MAX_ARG_LENGTH; i++) {
        cmdStruct->cmd[i] = '\0';
        cmdStruct->arg1[i] = '\0';
        cmdStruct->arg2[i] = '\0';
    }

    // Pointers for navigating and writing to fields
    const char *current = input;
    char *dest = cmdStruct->cmd; // Start with the cmd field
    int field = 0; // 0 = cmd, 1 = arg1, 2 = arg2

    // Iterate through the input string
    while (*current != '\0') {
        if (*current == ' ') { // Delimiter found
            if (dest != cmdStruct->cmd && dest != cmdStruct->arg1 && dest != cmdStruct->arg2) {
                // Null-terminate the current field
                *dest = '\0';

                // Move to the next field
                field++;
                if (field == 1) {
                    dest = cmdStruct->arg1;
                } else if (field == 2) {
                    dest = cmdStruct->arg2;
                } else {
                    break; // Stop if all fields are filled
                }
            }
            current++; // Skip the space
            continue;
        }

        // Copy the character if there's room in the current field
        if (field == 0 && (dest - cmdStruct->cmd) < MAX_ARG_LENGTH - 1) {
            *dest++ = *current;
        } else if (field == 1 && (dest - cmdStruct->arg1) < MAX_ARG_LENGTH - 1) {
            *dest++ = *current;
        } else if (field == 2 && (dest - cmdStruct->arg2) < MAX_ARG_LENGTH - 1) {
            *dest++ = *current;
        }

        current++;
    }

    // Null-terminate the last field
    if (dest != cmdStruct->cmd && dest != cmdStruct->arg1 && dest != cmdStruct->arg2) {
        *dest = '\0';
    }
}

void ServeCMD(command_t *cmd) {
    if (strcmp_lite(cmd->cmd, "lcd") == 0) {
        if (strcmp_lite(cmd->arg1, "-t") == 0) {
            ST7735_Message(0, 3, cmd->arg2, INT32_MAX);
        } 
        else if (strcmp_lite(cmd->arg1, "-b") == 0) {
            ST7735_Message(1, 3, cmd->arg2, INT32_MAX);
        } 
//        else if (strcmp_lite(cmd->arg1, "-d") == 0) {
//            //this is based on realmian lab2, i think we can use bottom half, after 4
//            #ifdef CALCULATE_TASK_JITTER
//                ST7735_Message(1, 6, "CLI: T1 Jitter: ", GetPeriodicTask1Jitter());
//                ST7735_Message(1, 7, "CLI: T2 Jitter: ", GetPeriodicTask2Jitter());
//            #endif
//        } 
        else {
            UART_OutString("stop fucking with the cli shitter\n");
        }
    } 
//    else if (strcmp_lite(cmd->cmd, "serial") == 0) {
//        if (strcmp_lite(cmd->arg1, "-jitter") == 0) {
//
//            #ifdef CALCULATE_TASK_JITTER
//                Print_JitterInfo(1);
//                //printf_lite("\n");
//                Print_JitterInfo(2);
//            #endif
//        } 
//        else {
//            UART_OutString("stop fucking with the cli shitter\r\n");
//        }
//    } 
//    else if (strcmp_lite(cmd->cmd, "os") == 0) {
//        if (strcmp_lite(cmd->arg1, "-t") == 0) {
//            ST7735_Message(1, 3, "MS TIME: ", OS_MsTime());
//        } else if (strcmp_lite(cmd->arg1, "-c") == 0) {
//            OS_ClearMsTime();
//        } else {
//            UART_OutString("stop fucking with the cli shitter\n");
//        }
//    } 
    else if (strcmp_lite(cmd->cmd, "led") == 0) {
        if (strcmp_lite(cmd->arg1, "-on") == 0) {
            GPIO_PORTF_DATA_R |= 0x02; //turn on pf2, blue led
        } else if (strcmp_lite(cmd->arg1, "-off") == 0) {
            GPIO_PORTF_DATA_R &= ~(0x02);//turn off pf2
        } else {
            UART_OutString("stop fucking with the cli shitter\n");
        }
    }

    //ls
    else if (strcmp_lite(cmd->cmd, "ls") == 0) {
       CLI_ls(); 
    }
    
    //cat
    else if (strcmp_lite(cmd->cmd, "cat") == 0) {
        CLI_cat(cmd->arg1);
    }

    //touch
    else if (strcmp_lite(cmd->cmd, "touch") == 0) {
        CLI_touch(cmd->arg1);           
    } 

    //write
    else if (strcmp_lite(cmd->cmd, "write") == 0) {
        CLI_write(cmd->arg1, cmd->arg2); 
    }
    
    //rm
    else if (strcmp_lite(cmd->cmd, "rm") == 0) {
        CLI_rm(cmd->arg1);
    } 

    //cd
    else if (strcmp_lite(cmd->cmd, "cd") == 0) {
        CLI_cd(cmd->arg1);
    } 

    //cd
    else if (strcmp_lite(cmd->cmd, "pwd") == 0) {
        CLI_pwd();
    } 

    //mount
    else if (strcmp_lite(cmd->cmd, "mount") == 0) {
        CLI_mount();
    } 

    //umount
    else if (strcmp_lite(cmd->cmd, "umount") == 0) {
        CLI_umount();
    } 
    
    //mkfs
    else if (strcmp_lite(cmd->cmd, "mkfs") == 0) {
        CLI_mkfs();
    }

    else if (strcmp_lite(cmd->cmd, "mkdir") == 0) {
        CLI_mkdir(cmd->arg1);
    } 

    //exec
    //else if (strcmp(cmd->cmd, "exec") == 0) {
    //    CLI_exec(cmd->arg1);
    //} 

    else {
        UART_OutString("stop fucking with the cli shitter\n");
    }

}

// *********** Command line interpreter (shell) ************
void Interpreter(void) { 
    while (1) {
        while (RxFifo_Get(&inChar) == 0) {
            OS_Sleep(50);
        }
        int i = 0;
        i++;
        if (inChar == '\n') {
            // Null-terminate the command string
            cmdStrBuf[cmdStrBuf_idx++] = '\0';
            //ST7735_Message(0, 3, "mesg rcvd", 10);
            //UART_OutString("Message Recieved\r\n");
            // Parse and execute the command
            ParseCMD(cmdStrBuf, &cmdStruct);  // Pass the command string
            ServeCMD(&cmdStruct); // Pass the parsed command struct

            cmdStrBuf_idx = 0;
        } else {
            // Store the character in the command buffer

            if (cmdStrBuf_idx < sizeof(cmdStrBuf) - 1) {
                cmdStrBuf[cmdStrBuf_idx++] = inChar;
            }else{
                // Command buffer overflow, send an error and reset
                UART_OutString("stop fucking with the cli shitter\n");
                cmdStrBuf_idx = 0;  // Reset the index
            }
        }
    }
}



