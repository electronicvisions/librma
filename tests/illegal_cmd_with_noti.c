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

#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <rma2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pmap.h>
#include <fcntl.h>

#define DEBUG(a)
//#define DEBUG(a) a

//variables below are set-up using the PMAP dirver in function setup_physical_buffer()
uint64_t buffer_size;//=4096;//4*1024*1024;
//pointer to the mmap physical buffer
uint64_t* send_buffer;

RMA2_NLA send_nla;
//physical address of the buffer
RMA2_Region* send_nla_buffer_region;

//pointer to the mmap physical buffer
uint64_t* recv_buffer;

RMA2_NLA recv_nla;
//physical address of the buffer
RMA2_Region* recv_nla_buffer_region;

void setup_buffer(RMA2_Port port, uint64_t** buffer, RMA2_Region** buffer_region, int buffer_size,int offset)
{
  int result;
  RMA2_ERROR rc;
  RMA2_NLA nla;
  
  result=posix_memalign((void**)buffer,4096,buffer_size+offset);
  if (result!=0)    
    {
      perror("Memory Buffer allocation failed. Bailing out.");
      abort();
    }
  *buffer+=offset;
//   *buffer=malloc(buffer_size);
//   if (*buffer==NULL)
//   {
//     fprintf(stderr,"Memory Buffer allocation failed. Bailing out.\n");
//     abort();
//   }
  rc=rma2_register(port,*buffer, buffer_size, buffer_region);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"Error while registering memory. Bailing out!\n");
    abort();
  }
  rma2_get_nla(*buffer_region, 0, &nla);
  printf("in setup_buffer: nla is %lx\n",nla);
}

RMA2_Descriptor __static_desc __attribute__ ((aligned (16)));

RMA2_ERROR rma2_post_illegal(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
                            RMA2_Command_Modifier modifier)
{
  RMA2_NLA src;
  
  rma2_get_nla(src_region, src_offset, &src);
  DEBUG(printf("Computed Source address is: %lx\n",src);)
  rma2_desc_set_put_get(&__static_desc,handle,4 /*RMA2_QW_PUT*/, size, spec,
                        modifier,src,dest_address);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
 
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
  uint32_t bytes;
  int i;
  uint64_t upper_boarder;
  uint64_t send_offset=0;
  uint64_t bytes_to_send=0;
  uint64_t noti_number;
  RMA2_Notification* notification;
  uint32_t requests=0;
  int trans;

  if (argc<2)
  {
    fprintf(stderr,"Erroneous commandline. Must provide 1 parameter: number of bytes to write.\n");
    abort();
  }
  //dest_nodeid=strtol(argv[1],0,0);
  //cmd_type=strtol(argv[1],0,0);
  bytes=strtol(argv[1],0,0);
  
  conn_type= RMA2_CONN_DEFAULT;    
  
  
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
  printf("Connection to node %d, vpid %d requested. Using connection type of %x.\n", dest_nodeid,dest_vpid, conn_type);
  rc=rma2_connect(port,dest_nodeid, dest_vpid ,conn_type, &handle);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA connect failed (%d)\n",rc);
    abort();
  }
  printf("connected!\n");
  buffer_size=bytes;
  setup_buffer(port, &send_buffer, &send_nla_buffer_region, buffer_size,0);
  setup_buffer(port,&recv_buffer, &recv_nla_buffer_region,buffer_size,0);//500);
  if (bytes % 8==0) upper_boarder=bytes/8;
  else upper_boarder=bytes/8+1;
  for (i=0;i<upper_boarder;i++)
  {
    send_buffer[i]=i;
    recv_buffer[i]=0xCAFEBABEl;
    //printf("SB: %lx RB: %lx\n",send_buffer[i],recv_buffer[i]);
  }

  rma2_get_nla(send_nla_buffer_region, 0, &send_nla);
  rma2_get_nla(recv_nla_buffer_region, 0, &recv_nla) ;
  printf("Got send nla of %lx (virtual is %p) and receive nla of %lx (virtual is %p)\n",send_nla,send_buffer,recv_nla,recv_buffer);
  printf("manipulated recv_nla to be %lx\n",recv_nla);
  send_offset=0;
  for (trans=0; trans<2; trans++)
  {
    bytes=buffer_size;
    send_offset=0;
    rma2_get_nla(recv_nla_buffer_region, 0, &recv_nla) ;
    requests=0;
     for (i=0;i<upper_boarder;i++)
    {
      send_buffer[i]=i;
      recv_buffer[i]=0xCAFEBABEl;
      //printf("SB: %lx RB: %lx\n",send_buffer[i],recv_buffer[i]);
    }
    while (bytes > 0 )
    {
      requests++;
      bytes_to_send= (bytes > 8388608) ? 8388608 : bytes;
      bytes=bytes - bytes_to_send;
      rc=rma2_post_illegal(port, handle, send_nla_buffer_region, send_offset, bytes_to_send, recv_nla,RMA2_ALL_NOTIFICATIONS,0);
      if (rc!=RMA2_SUCCESS)
      {
	fprintf(stderr,"rma2_post_put_qw() returned %d\n",rc);
	abort();
      }
      send_offset=send_offset+bytes_to_send;
      recv_nla=recv_nla+bytes_to_send;
    }
 
    noti_number=2 * requests;
    for (i=0;i<noti_number;i++)
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
    if (buffer_size % 8==0) upper_boarder=buffer_size/8;
    else upper_boarder=buffer_size/8+1;
    printf("upper boarder is %lx, bytes is %lx\n",upper_boarder,buffer_size);
    for (i=0;i<upper_boarder;i++)
    {
      //printf("SB: %lx RB: %lx\n",send_buffer[i],recv_buffer[i]);
      if (send_buffer[i]!=recv_buffer[i])
      {
				printf("Mismatch at %d: %lx %lx\n",i,send_buffer[i],recv_buffer[i]);
      }
    }
    printf("iteration done\n");
  }
  rma2_unregister(port,send_nla_buffer_region);
  rma2_unregister(port,recv_nla_buffer_region);
  //scanf("%d",&dummy);
  rc=rma2_disconnect(port,handle);
  rma2_close(port);
  return 0;
}
