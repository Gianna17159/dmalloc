DMalloc
===================

This project is a debugging memory allocator. It performs the following:
1. Tracks information about dynamic memory allocation.
2. Catches common memory programming errors, including freeing the same block twice and trying to allocate too much memory.
3. Detects a write outside a dynamically allocated block, (eg. writing 65 bytes into a 64-byte piece of memory).



Test program by running the following in the terminal:
make check SAN=1