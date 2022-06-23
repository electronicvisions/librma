#ifndef PINNED_BUFFER_H
#define PINNED_BUFFER_H


// Includes
//-------------
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <pmap.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
//#include <asm/io.h>

/**
 * Result of pinned buffer allocation
 */
typedef struct pinned_buffer_t {

        int valid; /// 1 if valid, 0 if an error occured and invalid
        uint64_t * buffer; /// The Virtual Address resident buffer for usage
        uint64_t size; /// The size of the buffer, provided by user for setup method to allocate the requested size
        uint64_t physical_buffer; /// The physical address


} pinned_buffer;

/**
 * allocates pinned memory from provided buffer->buffer_size
 * @param buffer
 * @return 0 if no error occured, -1 otherwise
 */
int setup_physical_buffer(pinned_buffer * buffer);


void release_physical_buffer(pinned_buffer * buffer);

/**
 * Pins the specified memory buffer to RAM, to avoid it beeing swaped, and make sure it is always perfectly available to the device
 * @param buffer
 * @param size
 */
int pin_memory(uint64_t * buffer,uint64_t size);

/**
 * Unpins the specified buffer
 * @param buffer
 * @param size
 * @return
 */
int unpin_memory(uint64_t * buffer,uint64_t size);

#endif
