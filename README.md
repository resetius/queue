# Queue

## Introduction
Queue is a C++ project implementing a queue data structure using mapped files, tailored for inter-process communication (IPC). It supports a single-writer, single-reader format, offering two implementations: lock-free and mutex-based.

## Features
- Queue implementation based on mapped files for efficient IPC.
- Two variants: lock-free and mutex-based for different use cases.
- `bench.cpp`: A benchmarking tool comparing the queue performance with standard and `vmsplice` pipes.

## Technology Stack
- C++
- CMake
- CMocka (for tests)

## Installation and Setup
```bash
# Clone the repository
git clone [repository URL]
```

# Build the project
```
mkdir build && cd build
cmake ..
make
```

## Usage
- The queue can be used for IPC in scenarios requiring a 1-writer, 1-reader format.
- Run benchmarking with `./bench` to compare performance against pipes.

## Benchmarks
To run benchmarks comparing queue performance with pipes:
```bash
./bench
```
