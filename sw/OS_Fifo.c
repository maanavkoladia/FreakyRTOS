/* ================================================== */
/*                      INCLUDES                      */
/* ================================================== */
#include "OS_Fifo.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "string_lite.h"
#include "printf.h"

#include "OS.h"

/* ================================================== */
/*            GLOBAL VARIABLE DEFINITIONS             */
/* ================================================== */

/* ================================================== */
/*                 FUNCTION DEFINITIONS               */
/* ================================================== */
bool Enqueue(uint8_t *inData, OS_FIFO_t* fifo){
    int16_t nextptr = fifo->head_OSFIFO + 1 >= fifo->size_OSFIFO ? 0 : fifo->head_OSFIFO + 1;
    if (nextptr == fifo->tail_OSFIFO) { // FIFO is full
        return false;
    }
    
    memcpy(fifo->OS_FIFO + (fifo->head_OSFIFO * fifo->elem_size), inData, fifo->elem_size); // Copy data into FIFO
    
    fifo->head_OSFIFO = nextptr;
    return true;
}

bool Dequeue(uint8_t* OutData, OS_FIFO_t* fifo){
    if (fifo->head_OSFIFO == fifo->tail_OSFIFO) { // FIFO is empty
        return false;
    }

    memcpy(OutData, fifo->OS_FIFO + (fifo->tail_OSFIFO * fifo->elem_size), fifo->elem_size); // Copy data out of FIFO

    fifo->tail_OSFIFO = fifo->tail_OSFIFO + 1 >= fifo->size_OSFIFO ? 0 : fifo->tail_OSFIFO + 1;
    return true;
}


OS_Return_t OS_Fifo_Put(uint8_t *data, OS_FIFO_t* fifo){
    // Instead of waiting, return immediately if FIFO is full (ISR-Safe)
    if (fifo->DataRoomAvailable->Value == 0) {
        return 0; // Fail immediately if FIFO is full (non-blocking for ISR)
    }
    
    OS_Wait(fifo->DataRoomAvailable); // Wait for space in FIFO
    OS_Wait(fifo->mutex); // Lock FIFO (critical section)

    bool success = Enqueue(data, fifo);

    OS_Signal(fifo->mutex); // Unlock FIFO
    OS_Signal(fifo->DataAvailable); // Signal that data is available

    return success ? 1 : 0;
}

uint8_t OS_Fifo_Get(uint8_t* dataOut, OS_FIFO_t* fifo){
    OS_Wait(fifo->DataAvailable); // Wait for data
    OS_Wait(fifo->mutex); // Lock FIFO

    bool success = Dequeue(dataOut, fifo);

    OS_Signal(fifo->mutex); // Unlock FIFO
    OS_Signal(fifo->DataRoomAvailable); // Signal that space is available

    return success ? 1 : 0; // Return 0 if failure (not ideal, but prevents undefined behavior)
}

int32_t OS_Fifo_Size(OS_FIFO_t* fifo){
    return fifo->size_OSFIFO;
}

OS_Return_t OS_Fifo_Init(uint32_t size, OS_FIFO_t* fifo, uint8_t* buffer, uint8_t elem_size, sema4_t semaphores[3]) {
    if (fifo == NULL || buffer == NULL || semaphores == NULL || size > OS_FIFO_MAXSIZE) {
        return OS_FAIL;
    }

    fifo->OS_FIFO = buffer;
    fifo->mutex = &semaphores[0];
    fifo->DataAvailable = &semaphores[1];
    fifo->DataRoomAvailable = &semaphores[2];

    fifo->head_OSFIFO = 0;
    fifo->tail_OSFIFO = 0;
    fifo->size_OSFIFO = size;  // +1 for distinguishing full vs empty
    fifo->elem_size = elem_size;

    OS_InitSemaphore(fifo->mutex, 1);                       // Mutex initially unlocked
    OS_InitSemaphore(fifo->DataAvailable, 0);               // FIFO initially empty
    OS_InitSemaphore(fifo->DataRoomAvailable, size);    // All slots available

    return OS_SUCCESS;
}


void OS_Fifo_Print(OS_FIFO_t* fifo){
    printf("FIFO Size: %d\n\r", fifo->size_OSFIFO);
    printf("FIFO Head: %d\n\r", fifo->head_OSFIFO);
    printf("FIFO Tail: %d\n\r", fifo->tail_OSFIFO);
    printf("FIFO Element Size: %d\n\r", fifo->elem_size);
    printf("FIFO Data Available: %d\n\r", fifo->DataAvailable->Value);
    printf("FIFO Data Room Available: %d\n\r", fifo->DataRoomAvailable->Value);

    printf("FIFO Buffer: ");
    for (int i = 0; i < fifo->size_OSFIFO; i++) {
        printf("%02X ", *(fifo->OS_FIFO + (i * fifo->elem_size)));
    }
}

