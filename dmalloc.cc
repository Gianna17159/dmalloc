#define DMALLOC_DISABLE 1
#include "dmalloc.hh"
#include <cassert>
#include <cstring>
#include <map>

dmalloc_stats g_stats = {0, 0, 0, 0, 0, 0, (long unsigned int) -1, 0};
std::map <void *, std::pair<const char*, long>> allocs_list;
/**
 * dmalloc(sz,file,line)
 *      malloc() wrapper. Dynamically allocate the requested amount `sz` of memory and 
 *      return a pointer to it 
 * 
 * @arg size_t sz : the amount of memory requested 
 * @arg const char *file : a string containing the filename from which dmalloc was called 
 * @arg long line : the line number from which dmalloc was called 
 * 
 * @return a pointer to the heap where the memory was reserved
 */
void* dmalloc(size_t sz, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
   
    void * mem_ptr = NULL;

    //integer overflow protection
    if (sz < (size_t) -1) {
        mem_ptr = base_malloc(sz + sizeof(metadata) + sizeof(char));
    }
    
    if (mem_ptr != NULL) {
        //update g_stats if malloc is successful
        g_stats.nactive++;
        g_stats.active_size += sz;
        g_stats.ntotal++;
        g_stats.total_size += sz;

        //updating heap min and max
        if ((uintptr_t) mem_ptr < g_stats.heap_min) {
            g_stats.heap_min = (uintptr_t) mem_ptr;
        }

        if ((uintptr_t) mem_ptr > g_stats.heap_max) {
            g_stats.heap_max = (uintptr_t) mem_ptr + sz + sizeof(metadata);
        }

        //accessing and setting metadata header
        metadata * math_ptr = (metadata *) mem_ptr;
        struct metadata data = {sz, 0, math_ptr};
        *math_ptr = data;
        metadata * payload_ptr = math_ptr + 1;
        data.header_ptr = math_ptr;

        //accessing and setting boundary value
        char * write_ptr = (char *) payload_ptr;
        char * boundary_ptr = write_ptr+sz;
        char secret = 'c';
        *boundary_ptr = secret;

        //add pointer to payload and file info to allocs_list map
        std::pair<const char *, long> alloc_loc;
        alloc_loc.first = file;
        alloc_loc.second = line;
        allocs_list[payload_ptr] = alloc_loc;

        return (void *) payload_ptr;
    }
    //if mem_ptr is null, indicate failure
    else {
        g_stats.nfail++;
        g_stats.fail_size += sz;
    }
    return mem_ptr;
}

/**
 * dfree(ptr, file, line)
 *      free() wrapper. Release the block of heap memory pointed to by `ptr`. This should 
 *      be a pointer that was previously allocated on the heap. If `ptr` is a nullptr do nothing. 
 * 
 * @arg void *ptr : a pointer to the heap 
 * @arg const char *file : a string containing the filename from which dfree was called 
 * @arg long line : the line number from which dfree was called 
 */
void dfree(void* ptr, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    
    if (ptr) {

        //check that ptr is in heap
        if ((uintptr_t) ptr > g_stats.heap_max || (uintptr_t) ptr < g_stats.heap_min) {
            fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, not in heap\n", 
            file, line, ptr);
            abort();
        }
        //locating metadata header
        metadata * math_ptr = (metadata *) ptr;
        metadata * data = math_ptr - 1;

        //check that pointer points to memory right after header
        if (data != data->header_ptr) {
            fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, not allocated\n", 
            file, line, ptr);
            abort();
        }

        //checks that memory has not been freed before
        if (data->freed == 1) {
            fprintf(stderr, "MEMORY BUG %s:%ld: invalid free of pointer %p, double free", 
            file, line, ptr);
            abort();
        }
        //locate secret value
        char * check_bd_ptr = (char *) ptr;
        char * boundary_ptr = check_bd_ptr + data->size;

        //check for wild write to area right after memory block
        if (*boundary_ptr != 'c') {
            fprintf(stderr, "MEMORY BUG: %s:%ld: detected wild write during free of pointer %p", 
            file, line, ptr);
            abort();
        }

        //free pointer, mark as freed, update stats, and remove from allocs_list
        base_free(ptr);
        data->freed = 1;
        g_stats.nactive --;
        g_stats.active_size -= data->size;
        allocs_list.erase(ptr);
    }
}

/**
 * dcalloc(nmemb, sz, file, line)
 *      calloc() wrapper. Dynamically allocate enough memory to store an array of `nmemb` 
 *      number of elements with wach element being `sz` bytes. The memory should be initialized 
 *      to zero  
 * 
 * @arg size_t nmemb : the number of items that space is requested for
 * @arg size_t sz : the size in bytes of the items that space is requested for
 * @arg const char *file : a string containing the filename from which dcalloc was called 
 * @arg long line : the line number from which dcalloc was called 
 * 
 * @return a pointer to the heap where the memory was reserved
 */
void* dcalloc(size_t nmemb, size_t sz, const char* file, long line) {
    
    void * ptr = NULL;

    //overwrite protection
    if ((nmemb * sz) / sz == nmemb && (nmemb * sz) / nmemb == sz) {
        ptr = dmalloc(nmemb * sz, file, line);
    }
    
    if (ptr) {
        memset(ptr, 0, nmemb * sz);
    }
    else {
        g_stats.nfail++;
    }
    return ptr;
}

/**
 * get_statistics(stats)
 *      fill a dmalloc_stats pointer with the current memory statistics  
 * 
 * @arg dmalloc_stats *stats : a pointer to the the dmalloc_stats struct we want to fill
 */
void get_statistics(dmalloc_stats* stats) {
    // Stub: set all statistics to enormous numbers
    memset(stats, 255, sizeof(dmalloc_stats));
    // Your code here.
    memcpy(stats, &g_stats, sizeof(g_stats));
}

/**
 * print_statistics()
 *      print the current memory statistics to stdout       
 */
void print_statistics() {
    dmalloc_stats stats;
    get_statistics(&stats);

    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

/**  
 * print_leak_report()
 *      Print a report of all currently-active allocated blocks of dynamic
 *      memory.
 */
void print_leak_report() {
    //no memory leaks
     if (allocs_list.size() == 0) {
        return;
    }
    //pointer to first element in allocs_list map
    std::map<void*, std::pair<const char*, long>>::iterator itr = allocs_list.begin();
   
    while (itr != allocs_list.end()) {
        void * mem_ptr = itr->first;

        //find file and line saved in key value pair
        const char * file = (itr->second).first;
        long line = (itr->second).second;

        //find size stored in metadata header
        metadata * math_ptr = (metadata *) mem_ptr;
        metadata * data_ptr = math_ptr - 1;
        int size = data_ptr->size;


        fprintf(stdout, "LEAK CHECK: %s:%ld: allocated object %p with size %d\n", 
        file, line, mem_ptr, size);
        ++itr;
    }
}
