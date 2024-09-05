#ifndef CHANNEL_H
#define CHANNEL_H

#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>


// An opaque type for a channel implemented a synchronous ring buffer
// guarded by a mutex and two semaphores.
// The channel is used to send and receive data between threads.
// The channel is closed when no more data can be sent to it.
// Its buffer has a fixed size and blocks when full. So, if you send data beyond the buffer size, the channel will block.
// Thus you should run the receiver in a separate thread to drain the channel before sending any more data.
// The channel is thread-safe and can be used concurrently by multiple threads.
typedef struct channel channel_t;

// Initialize a new channel
channel_t* channel_create();

// Send data to the channel
bool channel_send(channel_t* ch, void* data);

// Receive data from the channel
void* channel_receive(channel_t* ch);

// Close the channel
void channel_close(channel_t* ch);

// Destroy the channel and free resources
void channel_destroy(channel_t* ch);

#endif  // CHANNEL_H
