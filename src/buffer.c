#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"

// Put `element' in buffer tail and increment its tail index
void buffer_put(Buffer* b, int element){
    pthread_mutex_lock(&b->mutex);

    // waiting for size != CAP
    while(b->size == BUFFER_CAPACITY){
        pthread_cond_wait(&b->not_full, &b->mutex);
    }

    b->arr[b->tail] = element;
    b->tail = (b->tail + 1) % BUFFER_CAPACITY;
    b->size++;

    // signaling to other threads waiting for non empty buffer
    pthread_cond_signal(&b->not_empty);

    pthread_mutex_unlock(&b->mutex);
}

// Get `ret` element from the head of the buffer and increment its head index
int buffer_get(Buffer* b, int* element){
    pthread_mutex_lock(&b->mutex);

    // Semantic: buffer is empty, but since it is not closed, wait for other work to be inserted, or for the buffer to be closed
    while(b->size == 0 && !b->closed){
        pthread_cond_wait(&b->not_empty, &b->mutex);
    }

    // Semantic: buffer is empty and closed, so there will be no element to get, just terminate
    if(b->size == 0 && b->closed){
        pthread_mutex_unlock(&b->mutex);
        return 0;
    }

    // Semantic: size != for sure, so get the element from the buffer
    *element = b->arr[b->head];
    b->head = (b->head + 1) % BUFFER_CAPACITY;
    b->size--;

    pthread_cond_signal(&b->not_full);

    pthread_mutex_unlock(&b->mutex);
    return 1;
}

// init buffer struct values (state values and thread primitives)
void buffer_init(Buffer* b){
    b->size = 0;
    b->head = 0;
    b->tail = 0;
    b->closed = 0;

    int err0 = pthread_mutex_init(&b->mutex, NULL);
    if(err0 != 0){
        fprintf(stderr, "Error while initing mutex: %d", err0);
        exit(-1);
    }

    int err1 = pthread_cond_init(&b->not_empty, NULL);
    if(err1 != 0){
        fprintf(stderr, "Error while initing cond: %d", err1);
        exit(-1);
    }

    int err2 = pthread_cond_init(&b->not_full, NULL);
    if(err2 != 0){
        fprintf(stderr, "Error while initing cond: %d", err2);
        exit(-1);
    }
}

// set the entire buffer to a default value
void buffer_set(Buffer* b, int val){
    for(size_t i = 0; i < BUFFER_CAPACITY; i++){
        b->arr[i] = val;
    }
}

// deallocate buffer resources, both state values and thread primitives
void buffer_destroy(Buffer* b){
    b->size = 0;
    b->head = 0;
    b->tail = 0;

    int err0 = pthread_mutex_destroy(&b->mutex);
    if(err0 != 0){
        fprintf(stderr, "Error while destroying mutex: %d", err0);
        exit(-1);
    }

    int err1 = pthread_cond_destroy(&b->not_empty);
    if(err1 != 0){
        fprintf(stderr, "Error while destoying cond: %d", err1);
        exit(-1);
    }

    int err2 = pthread_cond_destroy(&b->not_full);
    if(err2 != 0){
        fprintf(stderr, "Error while destroying cond: %d", err2);
        exit(-1);
    }
}

void buffer_print_internal_state(Buffer* b){
    pthread_mutex_lock(&b->mutex);
    printf("size: %zu\n head: %zu\ntail: %zu\nbuffer:\n", b->size, b->head, b->tail);
    for(size_t i = 0; i < BUFFER_CAPACITY; i++){
        printf("%d ", b->arr[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&b->mutex);
}

void buffer_close(Buffer* b){
    pthread_mutex_lock(&b->mutex);
    b->closed = 1;
    pthread_cond_broadcast(&b->not_empty);
    pthread_mutex_unlock(&b->mutex);
}
