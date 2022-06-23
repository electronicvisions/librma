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
#ifdef MICEXTOLL
uint64_t buffer_size=4096;
#else
uint64_t buffer_size=4*1024*1024;
#endif
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
  
  printf("buffer_size is %d, offset is %d\n",buffer_size,offset);
  result=posix_memalign((void**)buffer,4096,buffer_size+offset);
  if (result!=0)    
    {
      perror("Memory Buffer allocation failed. Bailing out.");
      abort();
    }
  *buffer+=(offset/8);
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

// void setup_buffer(RMA2_Port port, uint64_t** buffer, RMA2_Region** buffer_region, int buffer_size)
// {
//   int result;
//   uint64_t value;
//   RMA2_ERROR rc;
//   RMA2_NLA nla;
//   
//   *buffer=malloc(buffer_size);
//   if (*buffer==NULL)
//   {
//     fprintf(stderr,"Memory Buffer allocation failed. Bailing out.\n");
//     abort();
//   }
//   rc=rma2_register(port,*buffer, buffer_size, buffer_region);
//   if (rc!=RMA2_SUCCESS)
//   {
//     fprintf(stderr,"Error while registering memory. Bailing out!\n");
//     abort();
//   }
//   rma2_get_nla(*buffer_region, 0, &nla);
//   printf("in setup_buffer: nla is %lx\n",nla);
// }

int main(int argc, char** argv)
{
  RMA2_Port port;
  RMA2_Port port2;
  RMA2_ERROR rc;
  RMA2_VPID vpid;
  RMA2_VPID vpid2;
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
  
  conn_type= RMA2_CONN_DEFAULT;    
  //printf("Connection to node %d, vpid %d requested. Using connection type of %x.\n", dest_nodeid,dest_vpid, conn_type);
  rc=rma2_open(&port);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA open failed (%d)\n",rc);
    abort();
  }  
  rc=rma2_open(&port2);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA open failed (%d)\n",rc);
    abort();
  }  

  nodeid=rma2_get_nodeid(port);
  vpid=rma2_get_vpid(port);
  vpid2=rma2_get_vpid(port2);
  dest_vpid=vpid2;
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
  buffer_size=bytes;
  setup_buffer(port2, &send_buffer, &send_nla_buffer_region, buffer_size,0xb30);
  setup_buffer(port,&recv_buffer, &recv_nla_buffer_region,buffer_size,0xc00);
  for (i=0;i<buffer_size/8;i++)
  {
    send_buffer[i]=i;
    recv_buffer[i]=0;
  }

  rma2_get_nla(send_nla_buffer_region, 0, &send_nla);
  rma2_get_nla(recv_nla_buffer_region, 0, &recv_nla) ;
  printf("Got send nla of %lx and receive nla of %lx\n",send_nla,recv_nla);
  
  //scanf("%d",&dummy);
  
  
  if (cmd_type==0)
    rc=rma2_post_get_qw(port, handle, recv_nla_buffer_region, 0, bytes, send_nla,RMA2_RESPONDER_NOTIFICATION | RMA2_COMPLETER_NOTIFICATION|RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);
  else if (cmd_type==1)
    rc=rma2_post_get_bt(port, handle, recv_nla_buffer_region, 0, bytes, send_nla,0,0);
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
  //could check for notis here...
  if (bytes % 8==0) upper_boarder=bytes/8;
  else upper_boarder=bytes/8+1;
  for (i=0;i<upper_boarder;i++)
  {
    //printf("SB: %lx RB: %lx\n",send_buffer[i],recv_buffer[i]);
    if (send_buffer[i]!=recv_buffer[i])
    {
      printf("Mismatch at %d: %lx %lx\n",i,send_buffer[i],recv_buffer[i]);
    }
  }
  
  rma2_unregister(port2,send_nla_buffer_region);
  rma2_unregister(port,recv_nla_buffer_region);
  //scanf("%d",&dummy);
  rc=rma2_disconnect(port,handle);
  rma2_close(port);
  rma2_close(port2);
  return 0;
}
