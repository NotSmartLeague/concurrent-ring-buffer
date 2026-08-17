#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>
#include <stddef.h>

#ifndef BUFFER_CAPACITY
#define BUFFER_CAPACITY 5
#endif

/*
 * FIFO buffer implemented as a circular buffer.
 *
 * - `size` is the number of elements currently stored in the buffer.
 * - `head` is the index of the next element to be removed.
 * - `tail` is the index where the next element will be inserted.
 */
typedef struct {
    int arr[BUFFER_CAPACITY];
    size_t size;
    size_t head;
    size_t tail;

    int closed;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} Buffer;

void buffer_init(Buffer* b);
void buffer_put(Buffer* b, int element);
int buffer_get(Buffer* b, int* element);
void buffer_close(Buffer* b);
void buffer_destroy(Buffer* b);

void buffer_set(Buffer* b, int elem);
void buffer_print_internal_state(Buffer* b);

#endif
