#include <stdlib.h>
#include "channel.h"

#define CHANNEL_BUFFER_SIZE 100

typedef struct channel {
    void* buffer[CHANNEL_BUFFER_SIZE]; // Circular buffer
    int read_index;                    // Index to read from
    int write_index;                   // Index to write to
    int count;                         // Number of elements in the buffer
    pthread_mutex_t mutex;             // Mutex to protect the buffer
    sem_t empty;                       // Semaphore to track empty slots in the buffer
    sem_t full;                        // Semaphore to track filled slots in the buffer
    bool is_closed;                    // Flag to indicate if the channel is closed
} channel_t;


channel_t* channel_create() {
    channel_t* ch = (channel_t*)malloc(sizeof(channel_t));
    if (ch == NULL) {
        return NULL;
    }

    ch->read_index = 0;
    ch->write_index = 0;
    ch->count = 0;
    ch->is_closed = false;

    pthread_mutex_init(&ch->mutex, NULL);
    sem_init(&ch->empty, 0, CHANNEL_BUFFER_SIZE);
    sem_init(&ch->full, 0, 0);

    return ch;
}

bool channel_send(channel_t* ch, void* data) {
    if (ch->is_closed) {
        return false;
    }

    sem_wait(&ch->empty);           // Wait for an empty slot
    pthread_mutex_lock(&ch->mutex); // Lock the buffer

    // Check if the channel is closed after acquiring the lock
    // If closed, release the lock and post the semaphore
    if (ch->is_closed) {
        pthread_mutex_unlock(&ch->mutex);
        sem_post(&ch->empty);
        return false;
    }
    
    // Write data to the buffer
    ch->buffer[ch->write_index] = data;
    // Update the write index and count
    ch->write_index = (ch->write_index + 1) % CHANNEL_BUFFER_SIZE;
    ch->count++;

    pthread_mutex_unlock(&ch->mutex);
    sem_post(&ch->full); // Decrement the full semaphore to indicate data is available

    return true;
}

void* channel_receive(channel_t* ch) {
    sem_wait(&ch->full);            // Wait for a filled slot
    pthread_mutex_lock(&ch->mutex); // Lock the buffer before pulling data from it

    // If count is zero, the channel is closed. Return NULL.
    if (ch->count == 0) {
        pthread_mutex_unlock(&ch->mutex);
        sem_post(&ch->full);
        return NULL;
    }
    
    // Read data from the buffer at the read index
    void* data = ch->buffer[ch->read_index];
    ch->read_index = (ch->read_index + 1) % CHANNEL_BUFFER_SIZE; // Update the read index
    ch->count--; // Decrement the count to indicate one less element in the buffer
    
    // Unlock the buffer and post the empty semaphore
    pthread_mutex_unlock(&ch->mutex);
    sem_post(&ch->empty); // Increment the empty semaphore to indicate an empty slot

    return data;
}

void channel_close(channel_t* ch) {
    pthread_mutex_lock(&ch->mutex);
    ch->is_closed = true;
    pthread_mutex_unlock(&ch->mutex);
}

void channel_destroy(channel_t* ch) {
    pthread_mutex_destroy(&ch->mutex);
    sem_destroy(&ch->empty);
    sem_destroy(&ch->full);
    free(ch);
}


#include <stdio.h>

// Receive numbers from the channel in a separate thread
// because sending beyond the available slots will block
// the main thread.
// This should be called before sending numbers to the channel.
void *receive_numbers(void *arg) {
    channel_t* ch = (channel_t*)arg;
    for (int i = 0; i < 1000; i++) {
        int* data = (int*)channel_receive(ch);
        printf("%d\n", *data);
        free(data);
    }
    return NULL;
}

int main() {
    channel_t* ch = channel_create();

    // Send data to the channel
    channel_send(ch, "Hello");
    channel_send(ch, "World");

    // Receive data from the channel
    printf("%s\n", (char*)channel_receive(ch));
    printf("%s\n", (char*)channel_receive(ch));
    
    // create a thread to receive numbers
    pthread_t thread;
    pthread_create(&thread, NULL, receive_numbers, ch);

    for (int i = 0; i < 1000; i++) {
        int *data = (int*)malloc(sizeof(int));
        *data = i;
        channel_send(ch, data);
    }

    // Wait for the thread to finish
    pthread_join(thread, NULL);

    // Close the channel
    channel_close(ch);

    // Destroy the channel
    channel_destroy(ch);

    return 0;
}


