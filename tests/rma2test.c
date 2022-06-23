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
#include "rma2test.h"
#include "time.h" 


uint64_t time_start,time_stop;
double usec;
double bw;

unsigned char* send_buffer;
unsigned char* recv_buffer;

RMA2_Nodeid pp_dest_node;
RMA2_VPID dest_vpid;
RMA2_Nodeid own_node;
RMA2_VPID own_vpid;

RMA2_Region* send_region;
RMA2_Region* recv_region;
RMA2_NLA dest_address;
RMA2_NLA own_nla;

RMA2_Port port;
RMA2_Handle handle;
#ifdef PHYS_NOTI
RMA2_Handle phys_handle;
#endif

RMA2_ERROR rc;

double cycles_per_usec;

int mode;

FILE* logfd=0;
char logbase[32];
char logname[255];

#ifdef LS3BEXTOLL
volatile unsigned char *ls3_rdtsc_vaddr;
int init_rdtsc (void)
{
  int fd;

  fd = open("/dev/mem", O_RDONLY);
  if(fd<0){
    printf("Error open /dev/mem for RDTSC\n");
    return -1;
  }

  ls3_rdtsc_vaddr = (unsigned char *)mmap(0, 0x800, PROT_READ, MAP_PRIVATE, fd, 0x3ff00000);
  if(ls3_rdtsc_vaddr == MAP_FAILED){
    printf("Error mmmaping for RDTSC\n");
    return -1;
  }
  ls3_rdtsc_vaddr =  (unsigned char *)((unsigned long)ls3_rdtsc_vaddr + 0x408);
  close(fd);

  return 0;
}
#else
 #define init_rdtsc() 0
#endif


void init(int argc, char** argv)
{

  struct tm *tm;
  time_t t;
  DEBUG(uint32_t size;)
  int result;
  
  result=init_rdtsc();
  if (result!=0)
     abort();
  
  DEBUG(printf("Get the time..\n");)
  t=time(0);
  DEBUG(printf("Convert...\n");)
  tm=localtime(&t);
  DEBUG(printf("Create base log file name ...\n");)
  strftime(logbase, 32,"_%d%m%y_%H%M.out" ,tm);

 if (argc<3) 
    {
      printf("Usage: ./rma2test <MHz CPU> <mode <dest_node> <dest_vpid> - mode is either 0 (pong) or 1 (ping). Pong must be started first, then destination must be given.\n");
      abort();
    }  
 cycles_per_usec=strtol(argv[1],0,0);
 mode=strtol(argv[2],0,0);
 if (mode!=0)
    {
      printf("Ping Mode.\n");
      if (argc!=5) 
        {
          printf("Usage: ./rma2test <MHz CPU> <mode> <dest_node> <dest_vpid> - mode is either 0 (pong) or 1 (ping). Pong must be started first, then destination must be given.\n");
          abort();
        }    
 
      pp_dest_node=strtol(argv[3],0,0);
      dest_vpid=strtol(argv[4],0,0);
      DEBUG(printf("Given peer is %u/%u\n", pp_dest_node, dest_vpid);)
    }
  else
    {
      printf("Pong mode.\n");
    }

  //printf("8 given destination (%u/%u).\n",pp_dest_node,dest_vpid);
  DEBUG(printf("Allocating and registering send and receive buffers of %u bytes.\n",MB8);)
  send_buffer=malloc(MB8);
  if (send_buffer==0)
    {
      perror("Allocation of send buffer failed.");
      abort();
    }
  recv_buffer=malloc(MB8);
  if (send_buffer==0)
    {
      perror("Allocation of recv buffer failed.");
      abort();
    }
  memset(send_buffer, 0, MB8);
  memset(recv_buffer, 0, MB8);
  //printf("7 given destination (%u/%u).\n",pp_dest_node,dest_vpid);
  rc=rma2_open(&port);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"RMA open failed (%d)\n",rc);
      abort();
    }
  //printf("6 given destination (%u/%u).\n",pp_dest_node,dest_vpid);
  rc=rma2_register(port,send_buffer, MB8, &send_region);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Send Buffer registration failed (%d).\n",rc);
      rma2_close(port);
      abort();
    }
	
  DEBUG(printf("Registered send region %p, got base nla %lu, offset is %u\n",send_buffer,send_region->nla,send_region->offset);)
  //printf("5 given destination (%u/%u).\n",pp_dest_node,dest_vpid);
  rc=rma2_register(port,recv_buffer, MB8, &recv_region);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Recv Buffer registration failed (%d).\n",rc);
      rma2_unregister(port,send_region);
      rma2_close(port);
      abort();
    }
  DEBUG(printf("Registered recv region, got base nla %lu\n",recv_region->nla);)
  rma2_get_nla(send_region, 0, &own_nla);
  DEBUG(printf("Sending NLA is %lx\n",own_nla);)
  rma2_get_nla(recv_region, 0, &own_nla);
  DEBUG(printf("Recv NLA is %lx\n",own_nla);)
  own_node=rma2_get_nodeid(port);
  own_vpid=rma2_get_vpid(port);
  //printf("4 given destination (%u/%u).\n",pp_dest_node,dest_vpid);
  printf("Running on node %u and vpid %u\n", own_node, own_vpid);
  DEBUG(printf("My base NLA is: %lx\n", own_nla);)

  DEBUG(size=rma2_get_queue_size(port);)
  DEBUG(printf("Running with a notification queue size of %u\n",size);)
  if (mode==0) // pong mode
  {
    printf("Start Ping process with the following commandline:\n");
    printf("\t./rma2test %4.0lf %d %d %d \n",cycles_per_usec,1,own_node,own_vpid);
  }
}

void shutdown(void)
{
  printf("Shutting down\n");
  DEBUG(printf("Unregister nla %lx\n",send_region->nla);)
  rma2_unregister(port,send_region);
  DEBUG(printf("Unregister nla %lx\n",recv_region->nla);)
  rma2_unregister(port,recv_region);
  rma2_disconnect(port,handle);
#ifdef PHYS_NOTI  
  rma2_disconnect(port,phys_handle);
#endif
  rma2_close(port);
  if (logfd!=0) fclose(logfd);
  printf("done\n");
}


void send_start(int cmd,  int repetitions)
{
  uint64_t value;
  uint8_t class;

  value=cmd;
  class=(uint8_t)(0xCD & 0xff);
#ifdef PHYS_NOTI  
  rma2_post_notification(port, phys_handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#else
  rma2_post_notification(port, handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#endif
  DEBUG(printf("Send start for command %x\n", cmd);)
}

void recv_start(int* cmd)
{
  RMA2_Notification* notip;
  uint64_t value;
  uint8_t class;

  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
    }
  pp_dest_node=rma2_noti_get_remote_nodeid(notip);
  dest_vpid=rma2_noti_get_remote_vpid(notip);
  *cmd=rma2_noti_get_notiput_payload(notip);//->operand.immediate_value;
//   printf("Got a noti for command %u\n", *cmd);
  if (rma2_noti_get_notiput_class(notip)/*->class*/!=0xCD)
    {
      fprintf(stderr, "Recv'd noti with wrong class (%x).\n",rma2_noti_get_notiput_class(notip)/*->class*/);
      shutdown();
      abort();
    }
  //printf("Got a noti from %u/%u\n",pp_dest_node,dest_vpid);
  rma2_noti_free(port,notip);
  
  //send ack
  value=*cmd;
  class=(uint8_t)(0xEF & 0xff);
#ifdef PHYS_NOTI  
  rma2_post_notification(port, phys_handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#else  
  rma2_post_notification(port, handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#endif
}

void wait_start_ack()
{
  RMA2_Notification* notip;
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
      abort();
    }
  if (rma2_noti_get_notiput_class(notip)/*->class*/!=0xEF)
    {
      fprintf(stderr, "Recv'd noti with wrong class (%u).\n",rma2_noti_get_notiput_class(notip)/*->class*/);
      shutdown();
      abort();
    }
  DEBUG(printf("Got a noti from %u/%u\n",pp_dest_node,dest_vpid);)
  rma2_noti_free(port,notip);
}


int main(int argc, char** argv)
{
  RMA2_Notification* notip;
  uint64_t value;
  uint8_t class;
  
  init(argc, argv);
  //  printf("given destination (%u/%u).\n",pp_dest_node,dest_vpid);
  if (mode==0) // pong
    {
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
          abort();
        }
      pp_dest_node=rma2_noti_get_remote_nodeid(notip);
      dest_vpid=rma2_noti_get_remote_vpid(notip);
      dest_address=rma2_noti_get_notiput_payload(notip);//->operand.immediate_value;
      DEBUG(printf("Received: remote node: %u, remote vpid %u, remote base address: %lx\n",
             pp_dest_node, dest_vpid, dest_address);)

      if (rma2_noti_get_notiput_class(notip)/*->class*/!=0xAB)
        {
          fprintf(stderr, "Recv'd noti with wrong class (%u).\n",rma2_noti_get_notiput_class(notip)/*->class*/);
          shutdown();
          abort();
        }
      //printf("Got a noti from %u/%u\n",pp_dest_node,dest_vpid);
      rma2_noti_free(port,notip);

      rc=rma2_connect(port, pp_dest_node,dest_vpid, /*RMA2_CONN_PHYSICAL*/ RMA2_CONN_DEFAULT,&handle);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr, "Connect failed(%d).\n",rc);
          shutdown();
          abort();
        }
#ifdef PHYS_NOTI
      rc=rma2_connect(port, pp_dest_node,dest_vpid, RMA2_CONN_PHYSICAL /*RMA2_CONN_DEFAULT*/,&phys_handle);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr, "Connect failed(%d).\n",rc);
          shutdown();
          abort();
        }
#endif
      rma2_get_nla(recv_region,0,&value);
      class=(uint8_t)(0xAB & 0xff);
#ifdef PHYS_NOTI      
      rma2_post_notification(port, phys_handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#else      
      rma2_post_notification(port, handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#endif      

      do_the_pong();
    }
  else //ping
    {
      printf("Connecting to given destination (%u/%u).\n",pp_dest_node,dest_vpid);
      rc=rma2_connect(port, pp_dest_node,dest_vpid, /*RMA2_CONN_PHYSICAL*/ RMA2_CONN_DEFAULT,&handle);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr, "Connect failed(%d).\n",rc);
          shutdown();
	  abort();
        }
#ifdef PHYS_NOTI        
      rc=rma2_connect(port, pp_dest_node,dest_vpid, RMA2_CONN_PHYSICAL /*RMA2_CONN_DEFAULT*/,&phys_handle);
      if (rc!=RMA2_SUCCESS)
       {
         fprintf(stderr, "Connect failed(%d).\n",rc);
         shutdown();
         abort();
       }
#endif       
      rma2_get_nla(recv_region,0,&value);
      class=(uint8_t)(0xAB & 0xff);
#ifdef PHYS_NOTI      
      rma2_post_notification(port, phys_handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#else      
      rma2_post_notification(port, handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#endif      
 
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
          abort();
        }
      pp_dest_node=rma2_noti_get_remote_nodeid(notip);
      dest_vpid=rma2_noti_get_remote_vpid(notip);
      dest_address=rma2_noti_get_notiput_payload(notip);//->operand.immediate_value;
      DEBUG(printf("Received: remote node: %u, remote vpid %u, remote base address: %lx\n",
             pp_dest_node, dest_vpid, dest_address);)
      if (rma2_noti_get_notiput_class(notip)/*->class*/!=0xAB)
        {
          fprintf(stderr, "Recv'd noti with wrong class (%u).\n",rma2_noti_get_notiput_class(notip)/*->class*/);
          shutdown();
          abort();
        }
      //printf("Got a noti from %u/%u\n",pp_dest_node,dest_vpid);
      rma2_noti_free(port,notip);
      
      do_the_ping();
    }

  shutdown();

  return 0;
}
