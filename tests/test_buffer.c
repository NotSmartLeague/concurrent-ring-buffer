#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"

typedef struct {
    Buffer* buffer;
    int elements;
    int* produced;
    int* consumed;
    pthread_mutex_t* count_mutex;
} TestState;

static void* producer_work(void* args){
    TestState* state = (TestState*) args;

    for(int i = 0; i < state->elements; i++){
        buffer_put(state->buffer, i);

        pthread_mutex_lock(state->count_mutex);
        (*state->produced)++;
        pthread_mutex_unlock(state->count_mutex);
    }

    return NULL;
}

static void* consumer_work(void* args){
    TestState* state = (TestState*) args;
    int element;

    while(buffer_get(state->buffer, &element)){
        pthread_mutex_lock(state->count_mutex);
        (*state->consumed)++;
        pthread_mutex_unlock(state->count_mutex);
    }

    return NULL;
}

static void run_producer_consumer_test(int n_producers, int n_consumers, int elements_per_producer){
    Buffer buffer;
    buffer_init(&buffer);

    int produced = 0;
    int consumed = 0;
    pthread_mutex_t count_mutex;
    assert(pthread_mutex_init(&count_mutex, NULL) == 0);

    pthread_t* producers = malloc((size_t) n_producers * sizeof(*producers));
    pthread_t* consumers = malloc((size_t) n_consumers * sizeof(*consumers));
    TestState* producer_states = malloc((size_t) n_producers * sizeof(*producer_states));
    TestState* consumer_states = malloc((size_t) n_consumers * sizeof(*consumer_states));

    assert(producers != NULL);
    assert(consumers != NULL);
    assert(producer_states != NULL);
    assert(consumer_states != NULL);

    for(int i = 0; i < n_consumers; i++){
        consumer_states[i] = (TestState) {
            .buffer = &buffer,
            .elements = 0,
            .produced = &produced,
            .consumed = &consumed,
            .count_mutex = &count_mutex
        };
        assert(pthread_create(&consumers[i], NULL, consumer_work, &consumer_states[i]) == 0);
    }

    for(int i = 0; i < n_producers; i++){
        producer_states[i] = (TestState) {
            .buffer = &buffer,
            .elements = elements_per_producer,
            .produced = &produced,
            .consumed = &consumed,
            .count_mutex = &count_mutex
        };
        assert(pthread_create(&producers[i], NULL, producer_work, &producer_states[i]) == 0);
    }

    for(int i = 0; i < n_producers; i++){
        assert(pthread_join(producers[i], NULL) == 0);
    }

    buffer_close(&buffer);

    for(int i = 0; i < n_consumers; i++){
        assert(pthread_join(consumers[i], NULL) == 0);
    }

    assert(produced == n_producers * elements_per_producer);
    assert(consumed == produced);

    free(producers);
    free(consumers);
    free(producer_states);
    free(consumer_states);

    assert(pthread_mutex_destroy(&count_mutex) == 0);
    buffer_destroy(&buffer);
}

static void test_one_producer_one_consumer(void){
    run_producer_consumer_test(1, 1, 50);
}

static void test_multiple_producers_multiple_consumers(void){
    run_producer_consumer_test(4, 3, 50);
}

static void test_closed_empty_buffer(void){
    Buffer buffer;
    int element;

    buffer_init(&buffer);
    buffer_close(&buffer);

    assert(buffer_get(&buffer, &element) == 0);

    buffer_destroy(&buffer);
}

static void test_closed_buffer_with_elements(void){
    Buffer buffer;
    int element;

    buffer_init(&buffer);
    buffer_put(&buffer, 10);
#if BUFFER_CAPACITY > 1
    buffer_put(&buffer, 20);
#endif
    buffer_close(&buffer);

    assert(buffer_get(&buffer, &element) == 1);
    assert(element == 10);
#if BUFFER_CAPACITY > 1
    assert(buffer_get(&buffer, &element) == 1);
    assert(element == 20);
#endif
    assert(buffer_get(&buffer, &element) == 0);

    buffer_destroy(&buffer);
}

int main(void){
    test_one_producer_one_consumer();
    test_multiple_producers_multiple_consumers();
    test_closed_empty_buffer();
    test_closed_buffer_with_elements();

    printf("All buffer tests passed (BUFFER_CAPACITY=%d).\n", BUFFER_CAPACITY);
    return 0;
}
