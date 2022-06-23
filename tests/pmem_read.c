#include <stdio.h>
#include <stdlib.h>
#include <rma2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pmap.h>
#include <fcntl.h>

//variables below are set-up using the PMAP dirver in function setup_physical_buffer()
uint64_t buffer_size=4*1024*1024;
//pointer to the mmap physical buffer
uint64_t* send_buffer;
//physical address of the buffer
uint64_t send_p_buffer;
//fd to access pmap driver
int send_pmap_fd;

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
  RMA2_VPID dest_vpid;
  RMA2_Nodeid dest_nodeid;
  RMA2_Connection_Options conn_type;
  uint64_t cmd_type;
  uint32_t bytes;
  int i;
  uint64_t upper_boarder;
  
  if (argc<3)
  {
    fprintf(stderr,"Erroneous commandline. Must provide 2 parameter: command type and number of bytes to read.\n");
    fprintf(stderr," Command type can be 0 - get qw, 1 - get bt\n");
    abort();
  }
  //dest_nodeid=strtol(argv[1],0,0);
  cmd_type=strtol(argv[1],0,0);
  bytes=strtol(argv[2],0,0);
  
  conn_type= RMA2_CONN_PHYSICAL;    
  //printf("Connection to node %d, vpid %d requested. Using connection type of %x.\n", dest_nodeid,dest_vpid, conn_type);
  rc=rma2_open(&port);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA open failed (%d)\n",rc);
    abort();
  }  
  nodeid=rma2_get_nodeid(port);
  vpid=rma2_get_vpid(port);
  dest_vpid=vpid;
  dest_nodeid=nodeid;
  printf("opened port %d on node %d\n",vpid, nodeid);
  
  rc=rma2_connect(port,dest_nodeid, dest_vpid ,conn_type, &handle);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA connect failed (%d)\n",rc);
    abort();
  }
  printf("connected!\n");
  //buffer_size=2*1024*1024;
  setup_physical_buffer(&send_pmap_fd, &send_buffer, &send_p_buffer, buffer_size);
  for (i=0;i<buffer_size/8;i++)
  {
    send_buffer[i]=i;
  }
  setup_physical_buffer(&recv_pmap_fd, &recv_buffer, &recv_p_buffer,buffer_size);
   for (i=0;i<buffer_size/8;i++)
  {
    recv_buffer[i]=0xcafebabel;
  }
  //printf("Accessing register at address 0x%lx on node %d: writing %lx\n",dest_rra_address,dest_nodeid,value);
  
  //scanf("%d",&dummy);
  
  
  if (cmd_type==0)
    rc=rma2_post_get_qw_direct(port, handle, recv_p_buffer, bytes, send_p_buffer ,0,0);
  else if (cmd_type==1)
    rc=rma2_post_get_bt_direct(port, handle, recv_p_buffer, bytes, send_p_buffer,0,0);
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
  sleep(1);
  if (bytes % 8==0) upper_boarder=bytes/8;
  else upper_boarder=bytes/8+1;
  for (i=0;i<upper_boarder;i++)
  {
    if (send_buffer[i]!=recv_buffer[i])
    {
      printf("Mismatch at %d: %lx %lx\n",i,send_buffer[i],recv_buffer[i]);
    }
  }
  //scanf("%d",&dummy);
  rc=rma2_disconnect(port,handle);
  rma2_close(port);
  return 0;
}
