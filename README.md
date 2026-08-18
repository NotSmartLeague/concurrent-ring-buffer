# Concurrent Ring Buffer

This project consists of a thread-safe ring buffer written in C using POSIX threads, focusing on concurrency, data race prevention and thread synchronization.

The buffer is used to implement a producer-consumer pattern:
- `producer` threads put elements into the buffer
- `consumer` threads retrieve elements from the buffer

This pattern has many real-world applications, such as:
- real-time messaging apps
- parallel computing in HPC
- job queues in operating systems

## Why a ring buffer?

A naive implementation of a FIFO buffer could be a simple array with this type of operation:
```
get():
    element = arr[size - 1]
    size--
    return element

put(element):
    shift_to_right(arr)
    arr[0] = element
    size++
``` 
That logic is simple and straightforward, but it makes `put` an O(n) operation, since all elements in the array need to be shifted.

A ring buffer uses a fixed-size array to store data and two indexes, `head` and `tail`, to keep track of read and write positions. Both indexes wrap around when they reach the end of the array.



## Design

`Buffer` stores integers in a circular array and tracks three main values:

- `size`: number of elements currently stored;
- `head`: index of the next element to read;
- `tail`: index where the next element will be written.

Access to the shared state is protected by a `pthread_mutex_t`.

Two condition variables coordinate producers and consumers:

- `not_empty`: consumers wait on it while the buffer is empty;
- `not_full`: producers wait on it while the buffer is full.

Conditions are always checked inside `while` loops because waking up does not guarantee that the required condition still holds when the thread reacquires the mutex.

This handles:
- changes to the shared state between waking up and reacquiring the mutex;
- spurious wakeups.

The `closed` flag indicates that no more elements will be produced. A consumer can still read elements already present after the buffer is closed; once the buffer is both closed and empty, `buffer_get` returns `0` and the consumer can terminate.

The open/closed semantics allow us to cleanly handle the case when there are no producers left, but there is still work to be done by the consumers.



## Buffer invariants

The implementation keeps these basic invariants while the buffer mutex is held:

- `0 <= size <= BUFFER_CAPACITY`;
- `head` and `tail` always stay in the range `[0, BUFFER_CAPACITY)`;
- a producer writes at `tail` and advances it modulo the capacity;
- a consumer reads at `head` and advances it modulo the capacity;
- producers wait while `size == BUFFER_CAPACITY`;
- consumers wait while `size == 0` and the buffer is still open.

## Trade-offs
In this implementation, the shared state of the buffer is protected by a single mutex. 

This keeps synchronization simple, but can introduce contention between producers and consumers, even when they are performing different operations on the buffer. This may become a performance bottleneck as concurrency increases. 

One possible improvement would be to explore finer-grained synchronization strategies. For this project, at least for now, I preferred to keep the synchonization model simple, and avoid introducing additional complexity before fully understanding fundamentals.

## Project structure

```text
README.md 
include/
    buffer.h (header declarations)
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

## Future work

Possible next steps are:
- introduce benchmarks to analyze buffer performance with different numbers of producers and consumers;
- use tools such as AddressSanitizer, UndefinedBehaviorSanitizer (UBSan), and ThreadSanitizer to detect memory errors, undefined behavior, and potential data races;
- explore different synchronization strategies and compare them using proper benchmarks.
