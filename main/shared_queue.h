#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include "freertos/queue.h"


// Define the structure for the data to be passed through the queue
typedef struct {
    uint8_t *data;        // Pointer to the data buffer
    size_t length;        // Length of the data buffer
} QueueData_t;

// Declare the queue handle as extern
extern QueueHandle_t shared_queue;

#endif // SHARED_QUEUE_H