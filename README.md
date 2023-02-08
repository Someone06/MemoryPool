# Memory Pool in C
This repository provides an implementation of a garbage-collected MemoryPool
using a simple non-concurrent, sequential mark-and-sweep algorithm.

## Low memory overhead, but slow
The MemoryPool only little bookkeeping data, at the expense of runtime.
The current implementation makes use of linked lists which need to be traveled
on every allocation and garbage collection. Memory compaction could alleviate
this to some extent, but is not implemented yet.

## Not Portable!
The MemoryPool uses only little bookkeeping information and makes excessive use
of pointer bit packing.
In particular, this implementation assumes 8-bit pointers, where the upper 16
bit of any pointer are unused and are zeros.
This assumption is true for current implementations of the x86_64 architecture,
but this might change in the (near) future.
**The code uses type punning and is NOT PORTABLE**, but it works on my machine,
*wink*. The current implementation also assumes that it sufficient to align any
type at a 8-bit boundary, which might not be sufficient for some data types. 

## TODOs
This implementation is a toy project for know and there is still a lot missing
to make the MemoryPool useful. This includes missing functionality, better
testing and developer tooling.

### Implement Memory Compaction
One of the main advantages of garbage collected languages is that they can do
memory compaction, meaning relocating all MemoryNodes and adjusting their
neighbour pointers accordingly. 

### Provide C++ interface
A proper C++ interface allows C++ program to make use of the memory pool.
C++ objects do not only require a storage location, but have a proper lifetime.
The interface must ensure that the lifetime of an object is started and that its
destructor is invoked upon before freeing the memory.

### Convert tests to Google Test
Providing a C++ interface also allows to easily test the code using the Google
Test testing framework. 
Using Google Test instead of a series of functions has several benefits
including better assertions, having all tests run to completion even if some
fail and better test result reporting.

### Fuzz testing
The memory pool includes a lot of complex, stateful low level code where errors
are easily missed by simple unit test. Fuzz testing allows to find errors that
result from odd usage patterns, that are hard to catch with manually written
tests.

### Allow the Memory Pool to grow
The size of the MemoryPool can currently only be set when the MemoryPool is
created, so users have to conservatively estimate the amount of memory that will
be needed. By allowing the MemoryPool to grow users can be more optimistic in
the amount of memory they will need, and just have the MemoryPool grow if the
guess was too optimistic. Memory compaction could in turn free some memory.

### Provide a reproducible testing environment
One hassle with C/C++ development is the use of different version of the same
tools. Building and testing the MemoryPool in a container could be a first step
to work around this.