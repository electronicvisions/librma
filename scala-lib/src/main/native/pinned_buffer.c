
#include "pinned_buffer.h"

int pmap_fd = 0;

int setup_physical_buffer(pinned_buffer * buffer) {


    // Prepare result
    //-------
    buffer->valid = 0;

    // Open PMAP
    //-------------------------
    if (pmap_fd==0) {
    	pmap_fd = open("/dev/pmap", O_RDWR);
		if (pmap_fd < 0) {
			perror("Opening PMAP device special file:");
			return -1;
		}
    }


    //set type to kernel alloced memory
    //-------------------------
    int result = ioctl(pmap_fd, PMAP_IOCTL_SET_TYPE, 0);
    if (result < 0) {
        perror("pmap ioctl PMAP_IOCTL_SET_TYPE failed:");
        return -1;
    }

    //set size
    //-------------------------
    result = ioctl(pmap_fd, PMAP_IOCTL_SET_SIZE, buffer->size);
    if (result < 0) {
        perror("pmap ioctl PMAP_IOCTL_SET_TYPE failed:");
        return -1;
    }

    // mmap the buffer
    //-------------------------
    buffer->buffer = (uint64_t*) mmap(0, /* preferred start */
            buffer->size, /* length in bytes */
    PROT_READ | PROT_WRITE, /* protection flags */
    MAP_SHARED, /* mapping flags */
    pmap_fd, /* file descriptor */
    0 //offset
            );
    if (buffer->buffer == MAP_FAILED ) {
        perror("Physcial buffer mmap failed");
        return -1;
    }

    // get physical address
    //-------------------------
    result = ioctl((pmap_fd), PMAP_IOCTL_GET_PADDR, &(buffer->physical_buffer));
    if (result < 0) {
        perror("pmap ioctl PMAP_IOCTL_GET_PADDR failed:");
        return -1;
    }
    buffer->valid = 1;

    printf("Physical Buffer set up at physical address: 0x%x\n",buffer->physical_buffer);

    return 0;

}

void release_physical_buffer(pinned_buffer * buffer) {

	munmap(buffer,buffer->size);

}

int pin_memory(uint64_t * buffer,uint64_t size) {


    if( mlock(buffer,size) <0) {
        perror("Locking memory failed");
        return -1;
    }

    //uint64_t phys_addr = virt_to_phys(buffer);
    printf("Pinned memory");

    return 0;


}

int unpin_memory(uint64_t * buffer,uint64_t size) {


    if(munlock(buffer,size)<0) {
        perror("Unlocking memory failed");
        return -1;
    }

    return 0;


}


