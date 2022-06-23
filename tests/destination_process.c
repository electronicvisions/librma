/***********************************************************************
*                                                                      *
* (C) 2008, Mondrian Nuessle, Computer Architecture Group,             *
* University of Heidelberg, Germany                                    *
* (C) 2011, Mondrian Nuessle, EXTOLL GmbH, Germany                     *
*                                                                      *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU Lesser General Public License as       *
* published by the Free Software Foundation; either version 3 of the   *
* License, or (at your option) any later version.                      *
*                                                                      *
* This program is distributed in the hope that it will be useful,      *
* but WITHOUT ANY WARRANTY; without even the implied warranty of       *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU Lesser General Public License for more details.                  *
*                                                                      *
* You should have received a copy of the GNU Lesser General Public     *
* License along with this program; if not, write to the Free Software  *
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 *
* USA                                                                  *
*                                                                      *
* For informations regarding this file contact nuessle@uni-mannheim.de *
*                                                                      *
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <rma2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pmap.h>
#include <fcntl.h>

//variables below are set-up using the PMAP dirver in function setup_physical_buffer()
uint64_t buffer_size=4*1024*1024;;

//pointer to the mmap physical buffer
uint64_t* recv_buffer;
//physical address of the buffer
uint64_t recv_p_buffer;
//fd to access pmap driver
int recv_pmap_fd;


void setup_physical_buffer(int* pmap_fd, uint64_t** buffer, uint64_t* p_buffer, int buffer_size)
{
  int result;
  uint64_t value;
  
  (*pmap_fd)=open("/dev/extoll/pmap",O_RDWR);
  if ( (*pmap_fd)<0)
  {
      perror("Opening PMAP device special file:");
      abort();
  }  
  //set type to kernel alloced memory
  value=0;
  result=ioctl((*pmap_fd), PMAP_IOCTL_SET_TYPE, value);
  if (result<0)
    {
      perror("pmap ioctl PMAP_IOCTL_SET_TYPE failed:");
     abort();
    }
  //set size
  value=buffer_size;
  result=ioctl((*pmap_fd), PMAP_IOCTL_SET_SIZE, value);
  if (result<0)
    {
      perror("pmap ioctl PMAP_IOCTL_SET_TYPE failed:");
      abort(); 
    }  
   //mmap the buffer
   (*buffer)=mmap(  0,	/* preferred start */
                 buffer_size,	/* length in bytes */
                 PROT_READ | PROT_WRITE,	/* protection flags */
                 MAP_SHARED,	/* mapping flags */
                 (*pmap_fd),	/* file descriptor */
                 0 //offset
               );
  if ((*buffer)==MAP_FAILED)
    {
      perror("Physcial buffer mmap failed");
      abort();
    }  
  //get physical address
  value=0;
  result=ioctl((*pmap_fd), PMAP_IOCTL_GET_PADDR, &value);
  if (result<0)
    {
      perror("pmap ioctl PMAP_IOCTL_GET_PADDR failed:");
     abort();
    }  
  *p_buffer=value;
  printf("mapped physcial buffer of size %d at virtual address %p, the physical address is %lx\n",
	 buffer_size, *buffer,*p_buffer);
  
}

int main(int argc, char** argv)
{
  RMA2_Port port;
  RMA2_ERROR rc;
  RMA2_VPID vpid;
  RMA2_Nodeid nodeid;
  RMA2_Handle handle;
  uint64_t i;
  RMA2_Notification* notification;
   
  rc=rma2_open(&port);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA open failed (%d)\n",rc);
    abort();
  }  
  nodeid=rma2_get_nodeid(port);
  vpid=rma2_get_vpid(port);
  printf("opened port %d on node %d\n",vpid, nodeid);  

 
  setup_physical_buffer(&recv_pmap_fd, &recv_buffer, &recv_p_buffer,buffer_size);
  for (i=0;i<buffer_size/8;i++)
  {
    recv_buffer[i]=i;
  }
  printf("Got physical buffer at vaddr %p and physical address %lx\n",recv_buffer,recv_p_buffer);

  printf("Now waiting for incoming requests. Exit with Ctrl-c\n");
  while(1)
  {
    rc=rma2_noti_get_block(port, & notification);
    if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"error in rma2_noti_get_block\n");
    }
    printf("\nGot Notification:\n");
    printf("-------------------------\n");
    rma2_noti_dump(notification);
    rma2_noti_free(port,notification);
    printf("-------------------------\n");
  }
  
//   //result=scanf("%ld",&dummy);
//   if (bytes % 8==0) upper_boarder=bytes/8;
//   else upper_boarder=bytes/8+1;
//   for (i=0;i<upper_boarder;i++)
//   {
//     if (send_buffer[i]!=recv_buffer[i])
//     {
//       printf("Mismatch at %ld: %lx %lx\n",i,send_buffer[i],recv_buffer[i]);
//     }
//   }
  
  rc=rma2_disconnect(port,handle);
  rma2_close(port);
  return 0;
}
