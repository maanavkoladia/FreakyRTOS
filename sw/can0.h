// *********can0.h ***************
// Runs on LM4F120/TM4C123
// Use CAN0 to communicate on CAN bus
// CAN0Rx PE4 (8) I TTL CAN module 0 receive.
// CAN0Tx PE5 (8) O TTL CAN module 0 transmit.

// Jonathan Valvano
// May 2, 2015

/* This example accompanies the books
   Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers, Volume 3,  
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2015

   Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers, Volume 2
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#ifndef __CAN0_H__
#define __CAN0_H__
#define CAN_BITRATE             1000000
#define CAN_MSG_LEN 8
#include <stdint.h>


// Returns true if receive data is available
//         false if no receive data ready
int CAN0_CheckMail(void);

// if receive data is ready, gets the data and returns true
// if no receive data is ready, returns false
int CAN0_GetMailNonBlock(uint8_t data[CAN_MSG_LEN]);

// if receive data is ready, gets the data 
// if no receive data is ready, it waits until it is ready
void CAN0_GetMail(uint8_t data[CAN_MSG_LEN]);

// Initialize CAN port
// rcvID is the receiver ID
// xmtID is the transmitter ID
void CAN0_Open(uint32_t rcvId, uint32_t xmtID);

// send 4 bytes of data to other microcontroller 
void CAN0_SendData(uint8_t data[CAN_MSG_LEN]);



#endif //  __CAN0_H__

