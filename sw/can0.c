// MCP2551 Pin1 TXD  ---- CAN0Tx PE5 (8) O TTL CAN module 0 transmit
// MCP2551 Pin2 Vss  ---- ground
// MCP2551 Pin3 VDD  ---- +5V with 0.1uF cap to ground
// MCP2551 Pin4 RXD  ---- CAN0Rx PE4 (8) I TTL CAN module 0 receive
// MCP2551 Pin5 VREF ---- open (it will be 2.5V)
// MCP2551 Pin6 CANL ---- to other CANL on network 
// MCP2551 Pin7 CANH ---- to other CANH on network 
// MCP2551 Pin8 RS   ---- ground, Slope-Control Input (maximum slew rate)
// 120 ohm across CANH, CANL on both ends of network
/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/hw_can.h"
#include "../inc/hw_ints.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_types.h"
#include "../inc/can.h"
#include "../inc/cpu.h"
#include "../inc/debug.h"
#include "../inc/interrupt.h"
#include "OS.h"
#include "../FreakyRTOS/can0.h"

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */
// reverse these IDs on the other microcontroller

#define NULL 0

uint32_t RCV_ID = 2;
uint32_t XMT_ID = 4;

// reverse these IDs on the other microcontroller

// Mailbox linkage from background to foreground
uint8_t static RCVData[CAN_MSG_LEN];
int static MailFlag;

sema4_t Can_Sema4;
/* ================================================== */
/*            FUNCTION PROTOTYPES (DECLARATIONS)      */
/* ================================================== */


/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */

//*****************************************************************************
//
// The CAN controller interrupt handler.
//
//*****************************************************************************
void CAN0_Handler(void){ 
    uint8_t data[CAN_MSG_LEN];
    uint32_t ulIntStatus, ulIDStatus;
    int i;
    tCANMsgObject xTempMsgObject;
    xTempMsgObject.pucMsgData = data;
    ulIntStatus = CANIntStatus(CAN0_BASE, CAN_INT_STS_CAUSE); // cause?
    
    if(ulIntStatus & CAN_INT_INTID_STATUS){  // receive?
        ulIDStatus = CANStatusGet(CAN0_BASE, CAN_STS_NEWDAT);

        for(i = 0; i < 32; i++){    //test every bit of the mask
            
            if( (0x1 << i) & ulIDStatus){  // if active, get data
                CANMessageGet(CAN0_BASE, (i+1), &xTempMsgObject, true);
                if(xTempMsgObject.ulMsgID == RCV_ID){
                    for(int i = 0; i < CAN_MSG_LEN; i++){
                        RCVData[i] = data[i];
                    }
                    MailFlag = true;   // new mail
                    OS_Signal(&Can_Sema4);
                }

            }

        }

    }

    CANIntClear(CAN0_BASE, ulIntStatus);  // acknowledge
}

//Set up a message object.  Can be a TX object or an RX object.
void static CAN0_Setup_Message_Object( uint32_t MessageID, \
                                    uint32_t MessageFlags, \
                                    uint32_t MessageLength, \
                                    uint8_t * MessageData, \
                                    uint32_t ObjectID, \
                                    tMsgObjType eMsgType){
    tCANMsgObject xTempObject;
    xTempObject.ulMsgID = MessageID;          // 11 or 29 bit ID
    xTempObject.ulMsgLen = MessageLength;
    xTempObject.pucMsgData = MessageData;
    xTempObject.ulFlags = MessageFlags;
    CANMessageSet(CAN0_BASE, ObjectID, &xTempObject, eMsgType);
}
// Initialize CAN port
void CAN0_Open(uint32_t rcvId, uint32_t xmtID){
    uint32_t volatile delay; 
    RCV_ID = rcvId; 
    XMT_ID = xmtID; 
    MailFlag = false;

    SYSCTL_RCGCCAN_R |= 0x00000001;  // CAN0 enable bit 0
    SYSCTL_RCGCGPIO_R |= 0x00000010;  // RCGC2 portE bit 4
    
    for(delay=0; delay<10; delay++){};

    GPIO_PORTE_AFSEL_R |= 0x30; //PORTE AFSEL bits 5,4
    // PORTE PCTL 88 into fields for pins 5,4
    GPIO_PORTE_PCTL_R = (GPIO_PORTE_PCTL_R&0xFF00FFFF)|0x00880000;
    GPIO_PORTE_DEN_R |= 0x30;
    GPIO_PORTE_DIR_R |= 0x20;
        
    CANInit(CAN0_BASE);
    CANBitRateSet(CAN0_BASE, 80000000, CAN_BITRATE);
    CANEnable(CAN0_BASE);

    // make sure to enable STATUS interrupts
    CANIntEnable(CAN0_BASE, CAN_INT_MASTER | CAN_INT_ERROR | CAN_INT_STATUS);
    // Set up filter to receive these IDs
    // in this case there is just one type, but you could accept multiple ID types
    CAN0_Setup_Message_Object(RCV_ID, MSG_OBJ_RX_INT_ENABLE, CAN_MSG_LEN, NULL, RCV_ID, MSG_OBJ_TYPE_RX);
    //CAN0_Setup_Message_Object(RCV_ID, MSG_OBJ_RX_INT_ENABLE | MSG_OBJ_USE_ID_FILTER, CAN_MSG_LEN, NULL, RCV_ID, MSG_OBJ_TYPE_RX);
    NVIC_EN1_R = (1 << (INT_CAN0 - 48)); //IntEnable(INT_CAN0);
    OS_InitSemaphore(&Can_Sema4, 0);
    return;
}

// send 4 bytes of data to other microcontroller 
void CAN0_SendData(uint8_t data[CAN_MSG_LEN]){
    // in this case there is just one type, but you could accept multiple ID types
    CAN0_Setup_Message_Object(XMT_ID, NULL, CAN_MSG_LEN, data, XMT_ID, MSG_OBJ_TYPE_TX);
}

// Returns true if receive data is available
//         false if no receive data ready
int CAN0_CheckMail(void){
  return MailFlag;
}
// if receive data is ready, gets the data and returns true
// if no receive data is ready, returns false
int CAN0_GetMailNonBlock(uint8_t data[CAN_MSG_LEN]){
    if(MailFlag){
        for(int i = 0; i < CAN_MSG_LEN; i++){
             data[i] = RCVData[i];
        }
        MailFlag = false;
        return true;
    }

    return false;
}

// if receive data is ready, gets the data 
// if no receive data is ready, it waits until it is ready
void CAN0_GetMail(uint8_t data[CAN_MSG_LEN]){
    OS_Wait(&Can_Sema4);
    //while(MailFlag==false){};
    for(int i = 0; i < CAN_MSG_LEN; i++){
            data[i] = RCVData[i];
    }
    MailFlag = false;
}

