#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "buffer.h"

#define N_CONSUMERS 3
#define N_PRODUCERS 2
#define ELEMENTS_FOR_PRODUCER 10



typedef struct{
    int id;
    int product_count;
    int* active_producers;
    Buffer* buffer;

    pthread_mutex_t* mutex;

} SharedState;


void* producer_work(void* args){
    SharedState* state = (SharedState*) args;
    
    Buffer* buffer = state->buffer;

    for(int i = 0; i < ELEMENTS_FOR_PRODUCER; i++){
        int next_int = i * state->product_count * (state->id + 1);

        buffer_put(buffer, next_int);

        pthread_mutex_lock(state->mutex);
        printf("Buffer state after Producer %d put element %d:\n", state->id, next_int);
        buffer_print_internal_state(buffer);
        printf("\n");
        pthread_mutex_unlock(state->mutex);

        state->product_count++;
    }

    pthread_mutex_lock(state->mutex);
    (*(state->active_producers))--;
    printf("[Producer %d]: decreased active_producers: %d\n", state->id, *(state->active_producers));

    // If there are no producer left, broadcast all consumer so they can terminate
    if(*(state->active_producers) == 0){
        buffer_close(buffer);
    }

    pthread_mutex_unlock(state->mutex);


    return NULL;
}

void do_work(int element){
    usleep(element);
}

void* consumer_work(void* args){
    SharedState* state = (SharedState*) args;

    Buffer* buffer = state->buffer;
    
    int element;
    while(buffer_get(buffer, &element)){
        pthread_mutex_lock(state->mutex);
        printf("Shared state of Consumer [%d]: active producers=%d\n", state->id, *(state->active_producers));
        printf("Buffer state after Consumer %d get %d:\n", state->id, element);
        buffer_print_internal_state(buffer);
        printf("\n");
        pthread_mutex_unlock(state->mutex);

        do_work(element);
    }

    return NULL;    
}

int main(void){
    Buffer b;
    buffer_init(&b);
    buffer_set(&b, -1);

    pthread_t consumers_t[N_CONSUMERS];
    pthread_t producers_t[N_PRODUCERS];

    SharedState states[N_CONSUMERS + N_PRODUCERS];
    
    int active_producers = N_PRODUCERS;

    pthread_mutex_t mutex;

    pthread_mutex_init(&mutex, NULL);
        
    // Creating producer threads
    for (int i = 0; i < N_PRODUCERS; i++) {
        states[i].id = i;
        states[i].product_count = 0;
        states[i].active_producers = &active_producers;
        states[i].buffer = &b;
        states[i].mutex = &mutex;

        int err = pthread_create(
            &producers_t[i],
            NULL,
            producer_work,
            &states[i]
        );

        if (err != 0) {
            fprintf(stderr,
                    "Error while creating Producer thread [%d]: %d\n",
                    i, err);
            exit(EXIT_FAILURE);
        }
    }


    // Creating consumer threads
    for (int i = 0; i < N_CONSUMERS; i++) {
        int state_index = N_PRODUCERS + i;

        states[state_index].id = i;
        states[state_index].product_count = 0;
        states[state_index].active_producers = &active_producers;
        states[state_index].buffer = &b;
        states[state_index].mutex = &mutex;

        int err = pthread_create(
            &consumers_t[i],
            NULL,
            consumer_work,
            &states[state_index]
        );

        if (err != 0) {
            fprintf(stderr,
                    "Error while creating Consumer thread [%d]: %d\n",
                    i, err);
            exit(EXIT_FAILURE);
        }
    }


    // Joining producer threads
    for (int i = 0; i < N_PRODUCERS; i++) {
        int err = pthread_join(producers_t[i], NULL);

        if (err != 0) {
            fprintf(stderr,
                    "Error while joining Producer thread [%d]: %d\n",
                    i, err);
            exit(EXIT_FAILURE);
        }
    }


    // Joining consumer threads
    for (int i = 0; i < N_CONSUMERS; i++) {
        int err = pthread_join(consumers_t[i], NULL);

        if (err != 0) {
            fprintf(stderr,
                    "Error while joining Consumer thread [%d]: %d\n",
                    i, err);
            exit(EXIT_FAILURE);
        }
    }
    buffer_destroy(&b);

    return 0;
}

