# Concurrent Ring Buffer

This project consists of a thread-safe ring buffer written in C using POSIX threads, focussing on concurrency, data race prevention and thread syncronization.

I've applied all these concepts implementing a producer-consumer pattern:
- `producers` threads are the ones who put stuff in the buffer
- `consumers` threads are the ones who get stuff from the buffer

This pattern has a lot of real-life application like:
- real time messaging apps
- HPC parallelism
- job queues in operating systems


## Design

`Buffer` stores integers in a circular array and tracks three main values:

- `size`: number of elements currently stored;
- `head`: index of the next element to read;
- `tail`: index where the next element will be written.

Access to the shared state is protected by a `pthread_mutex_t`.

Two condition variables coordinate producers and consumers:

- `not_empty`: consumers wait on it while the buffer is empty;
- `not_full`: producers wait on it while the buffer is full.

The `closed` flag indicates that no more elements will be produced. A consumer can still read elements already present after the buffer is closed; once the buffer is both closed and empty, `buffer_get` returns `0` and the consumer can terminate.

The open/closed semantic allow us to smartly manage the case when there are no producers left, but there is still work to do by the consumers.

## Buffer invariants

The implementation keeps these basic invariants while the buffer mutex is held:

- `0 <= size <= BUFFER_CAPACITY`;
- `head` and `tail` always stay in the range `[0, BUFFER_CAPACITY)`;
- a producer writes at `tail` and advances it modulo the capacity;
- a consumer reads at `head` and advances it modulo the capacity;
- producers wait while `size == BUFFER_CAPACITY`;
- consumers wait while `size == 0` and the buffer is still open.

## Project structure

```text
README.md 
include/
    buffer.h (header declaration)
src/
    buffer.c (actual buffer implementation)
    main.c (producer-consumer implementation)
tests/
    test_buffer.c
```

## Build and run

For now the project can be compiled directly with `gcc`:

```bash
gcc -Wall -Wextra -Wpedantic -pthread -Iinclude src/main.c src/buffer.c -o concurrent_ring_buffer
./concurrent_ring_buffer
```

## Tests

The test suite covers:

- one producer and one consumer;
- multiple producers and multiple consumers;
- equality between produced and consumed element counts;
- closing an empty buffer;
- closing a buffer that still contains elements;
- capacity equal to `1`.

Run the normal-capacity tests with:

```bash
gcc -Wall -Wextra -Wpedantic -pthread -Iinclude tests/test_buffer.c src/buffer.c -o test_buffer
./test_buffer
```

To exercise the same tests with capacity `1`:

```bash
gcc -Wall -Wextra -Wpedantic -pthread -DBUFFER_CAPACITY=1 -Iinclude tests/test_buffer.c src/buffer.c -o test_buffer_capacity_1
./test_buffer_capacity_1
```
