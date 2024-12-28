DMalloc
===================

This project is a debugging memory allocator. It performs the following:
1. Tracks information about dynamic memory allocation.
2. Catches common memory programming errors, including freeing the same block twice and trying to allocate too much memory.
3. Detects a write outside a dynamically allocated block, (eg. writing 65 bytes into a 64-byte piece of memory).
4. Reports memory leaks, printing a list of every object that has been allocated via dmalloc but not freed using dfree.

BASE_MALLOC/BASE_FREE
===================
This allocator behaves like malloc and free, but has the following additional properties:

1. base_free does not modify freed memory (the contents of freed storage remain unchanged until the storage is reused by a future base_malloc).
2. base_free never returns freed memory to the operating system

Test program by running the following in the terminal:
make check SAN=1