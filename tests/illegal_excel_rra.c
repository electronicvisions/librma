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
uint64_t buffer_size=2*1024*1024;
//pointer to the mmap physical buffer
uint64_t* buffer;
//physical address of the buffer
uint64_t p_buffer;
//fd to access pmap driver
int pmap_fd;

void setup_physical_buffer(void)
{
  int result;
  uint64_t value;
  
  pmap_fd=open("/dev/extoll/pmap",O_RDWR);
  if (pmap_fd<0)
  {
      perror("Opening PMAP device special file:");
      abort();
  }  
  //set type to kernel alloced memory
  value=0;
  result=ioctl(pmap_fd, PMAP_IOCTL_SET_TYPE, value);
  if (result<0)
    {
      perror("pmap ioctl PMAP_IOCTL_SET_TYPE failed:");
     abort();
    }
  //set size
  value=buffer_size;
  result=ioctl(pmap_fd, PMAP_IOCTL_SET_SIZE, value);
  if (result<0)
    {
      perror("pmap ioctl PMAP_IOCTL_SET_TYPE failed:");
      abort(); 
    }  
   //mmap the buffer
   buffer=mmap(  0,	/* preferred start */
                 buffer_size,	/* length in bytes */
                 PROT_READ | PROT_WRITE,	/* protection flags */
                 MAP_SHARED,	/* mapping flags */
                 pmap_fd,	/* file descriptor */
                 0 //offset
               );
  if (buffer==MAP_FAILED)
    {
      perror("Physcial buffer mmap failed");
      abort();
    }  
  //get physical address
  value=0;
  result=ioctl(pmap_fd, PMAP_IOCTL_GET_PADDR, &value);
  if (result<0)
    {
      perror("pmap ioctl PMAP_IOCTL_GET_PADDR failed:");
     abort();
    }  
  p_buffer=value;
  printf("mapped physcial buffer of size %lx at virtual address %p, the physical address is %lx\n",
	 buffer_size, buffer,p_buffer);
}

int main(int argc, char** argv)
{
  RMA2_Port port;
  RMA2_ERROR rc;
  RMA2_VPID vpid;
  RMA2_Nodeid nodeid;
  RMA2_Handle handle;
  RMA2_VPID dest_vpid;
  RMA2_Nodeid dest_nodeid;
  RMA2_Connection_Options conn_type;
  uint64_t dest_rra_address;
  uint64_t value;
  uint64_t cmd_type;
  RMA2_Notification* notification;
  
  if (argc<5)
  {
    fprintf(stderr,"Erroneous commandline. Must provide 3 parameter: destination node, registerfile address, value and command typeto write into register\n");
    fprintf(stderr," Command type can be 0 - put qw, 1 - imm put or 2 - put bt\n");
    abort();
  }
  dest_nodeid=strtol(argv[1],0,0);
  dest_rra_address=strtol(argv[2],0,0);
  value=strtol(argv[3],0,0);
  cmd_type=strtol(argv[4],0,0);
  dest_vpid=0;
  conn_type=  RMA2_CONN_PHYSICAL |  RMA2_CONN_RRA ; 
  printf("Connection to node %d, vpid %d requested. Using connection type of %x.\n", dest_nodeid,dest_vpid, conn_type);
  rc=rma2_open(&port);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA open failed (%d)\n",rc);
    abort();
  }  
  nodeid=rma2_get_nodeid(port);
  vpid=rma2_get_vpid(port);
  printf("opened port %d on node %d\n",vpid, nodeid);
  
  rc=rma2_connect(port,dest_nodeid, dest_vpid ,conn_type, &handle);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA connect failed (%d)\n",rc);
    abort();
  }
  printf("connected!\n");
  setup_physical_buffer();
  printf("Accessing register at address 0x%lx on node %d: writing %lx\n",dest_rra_address,dest_nodeid,value);
  
  //scanf("%d",&dummy);
  //write value into register..
  buffer[0]=value;
  if (cmd_type==0)
    rc=rma2_post_put_qw_direct(port, handle, p_buffer, 8, dest_rra_address,RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_EWA);
  else if (cmd_type==1)
    rc=rma2_post_put_bt_direct(port, handle, p_buffer, 8, dest_rra_address,RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_EWA);
  else if (cmd_type==2)
    rc=rma2_post_immediate_put(port, handle, 8, value,dest_rra_address , RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_EWA);
  else
  {
    fprintf(stderr,"Illegal command type %ld given on commandline\n",cmd_type);
    abort();
  }
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"rma2_post_put_qw() returned %d\n",rc);
    abort();
  }
    printf("now waiting for notification\n");
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
  //scanf("%d",&dummy);
  rc=rma2_disconnect(port,handle);
  rma2_close(port);
  return 0;
}
