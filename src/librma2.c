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


#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#define __USE_GNU
#include <dlfcn.h>

#include <assert.h>
#include "rma2.h"
#include "arch_define.h"


#include <rma2_ioctl.h>


//enable the code to handle wcbuf retries -> recommended
#define PACKET_BUFFER

//comment/uncomment to enable/disable debug output
#define DEBUG(a)
//#define DEBUG(a) a

//comment if you want to disable usage of SFENCE and instead use CLFLUSH instruction to flush write combining buffers
#define SFENCE


static unsigned long _extoll_page_size=0;
static unsigned _extoll_vpids_per_page=0;

RMA2_Descriptor __static_desc __attribute__ ((aligned (16)));

static inline void velo_clflush(volatile void *__p)
{
  asm volatile("clflush %0" : "+m" (*(char /*__attribute__((force))*/ /*__force*/ *)__p));
}

//uncomment the following line, if you want to enable the region manager of librma
//#define ENABLE_REGIONMANAGER

char* __rma2_command_strings[]= {
  "RMA2_CL_GET",
  "RMA2_QW_GET",
  "RESERVED",
  "RMA2_CL_PUT",
  "RMA2_QW_PUT",
  "RMA2_LOCK",
  "RMA2_IMM_PUT",
  "RMA2_NOTIFICATION_PUT"
};

char* __rma2_notification_strings[] = {
  "NONE",
  "RESPONDER",
  "COMPLETER",
  "RESPONDER | COMPLETER",
  "REQUESTER",
  "RESPONDER | REQUESTER",
  "COMPLETER | REQUESTER",
  "RESPONDER | COMPLETER | REQUESTER"
};

const RMA2_Class RMA2_CLASS_ANY = 0xff;
const RMA2_Nodeid RMA2_NODEID_ANY = 0xffff;
const RMA2_VPID RMA2_VPID_ANY = 0xffff;
const uint32_t RMA2_LOCK_ANY = 0xffffffff;
const RMA2_NLA RMA2_ANY_NLA=0xFFFFFFFFFFFFFFFF;

const uint32_t RMA2_DEFAULT_FLAG = 0x0;
const uint32_t RMA2_BLOCK_FLAG = 0x1;
const uint32_t RMA2_MATCH_LAST_NLA_FLAG = 0x2;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wsign-compare"
void  rma2_read_notiqueue(volatile uint64_t * map, uint32_t len)
{
  int i;
  
  if (len > (4096 / 8))
    len = 4096 / 8;
  printf("now reading completer mmap at %p\n", map);
  
  for (i = 0; i < len; i++) {
    printf("Memory on offset %d is %llX\n", i,
           (unsigned long long)map[i]);
  }
}
#pragma GCC diagnostic pop




RMA2_ERROR _rma2_get_size(RMA2_Port port, uint64_t *value)
{   
 int  result;
 result=ioctl(port->fd, RMA2_IOCTL_GET_NOTIQ_SIZE, value);
  if (result<0)
    {
      //perror("RMA ioctl RMA2_IOCTL_GET_NOTIQ_SIZE failed:");
      return RMA2_ERR_ERROR;      
    }
  return RMA2_SUCCESS;
}

RMA2_ERROR _rma2_set_size(RMA2_Port port, uint64_t bytes)
{
 int result;
 result=ioctl(port->fd, RMA2_IOCTL_SET_NOTIQ_SIZE, bytes);
  if (result<0)
    {
      //perror("RMA ioctl RMA2_IOCTL_SET_NOTIQ_SIZE failed:");
      return RMA2_ERR_ERROR;      
    }
  return RMA2_SUCCESS;
}

RMA2_ERROR _rma2_set_segment_size(RMA2_Port port, uint64_t bytes)
{
 int result;            
 result=ioctl(port->fd, RMA2_IOCTL_SET_NOTIQ_SEGMENT_SIZE, bytes);
  if (result<0)
    {
      //perror("RMA ioctl RMA2_IOCTL_SET_NOTIQ_SEGMENT_SIZE failed:");
      return RMA2_ERR_ERROR;      
    }
  return RMA2_SUCCESS;
}

RMA2_ERROR _rma2_queue_mmap(RMA2_Port port)
{
  int result;
  uint64_t value;
  uint64_t req_mapping_size;
  req_mapping_size= (_extoll_page_size <= 32*4096) ? 32*4096 : _extoll_page_size;
  
   port->queue.rp_wb=mmap(  0,	/* preferred start */
                           sizeof(RMA2_Notification) * port->queue.size + _extoll_page_size,	/* length in bytes */
                           PROT_READ | PROT_WRITE,	/* protection flags */
                           MAP_SHARED,	/* mapping flags */
                           port->fd,	/* file descriptor */
                           req_mapping_size + _extoll_page_size//offset
                           );
  if (port->queue.rp_wb==MAP_FAILED)
    {
      //perror("Notification queue mmap failed");
      return RMA2_ERR_MMAP;
    }
  port->queue.queue=(RMA2_Notification*)(port->queue.rp_wb+_extoll_page_size/sizeof(*(port->queue.rp_wb)));
  value=0;
  result=ioctl(port->fd, RMA2_IOCTL_SET_NOTIQ_WP, value);
  if (result<0)
    {
      //perror("RMA ioctl SET_NOTI_WP failed:");
      return RMA2_ERR_ERROR;      
    }

  result=ioctl(port->fd, RMA2_IOCTL_SET_NOTIQ_RP, value);
  if (result<0)
    {
      //perror("RMA ioctl SET_NOTI_RP failed:");
      return RMA2_ERR_ERROR;      
    }
 return RMA2_SUCCESS;
}

RMA2_ERROR _rma2_queue_munmap(RMA2_Port port)
{
  int err=munmap((void*)port->queue.rp_wb,sizeof(RMA2_Notification) * port->queue.size +_extoll_page_size);
  if (err) return RMA2_ERR_MMAP;
  return RMA2_SUCCESS;  
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
RMA2_ERROR rma2_open(RMA2_Port* port)
{
  int i, result;
  RMA2_Port p;
  uint64_t value;
  uint64_t req_mapping_size;
  
  _extoll_page_size= sysconf(_SC_PAGESIZE);
  _extoll_vpids_per_page=_extoll_page_size/4096;
  
  p = malloc(sizeof(RMA2_Endpoint));
  *port = p;
	
  (*port)->fd=open("/dev/extoll/rma2",O_RDWR);
  if ((*port)->fd<0)
    {
      //perror("Opening RMA device special file:");
      return RMA2_ERR_FD;
    }  
  //set segment size to 64k
  if ( (64*1024 >= _extoll_page_size) && (64*1024 % _extoll_page_size ==0) )
    value=64*1024;
  else //choose system page size
    value=_extoll_page_size;
  result=_rma2_set_segment_size(*port, value);
  if (result<0)
    {
      return RMA2_ERR_ERROR;      
    }
  // set notification size to 4*64k = 16k notifications = 256 kB Queue size
   if ( (256*1024 >= _extoll_page_size) && (256*1024 % _extoll_page_size ==0) )
    value=256*1024;
  else //choose system page size
    value=_extoll_page_size;
  result=_rma2_set_size(*port, value);
  if (result<0)
    {
      return RMA2_ERR_ERROR;      
    }
    
  result=_rma2_get_size(*port, &value); 
  if (result<0)
    {
      return RMA2_ERR_ERROR;      
    }
  (*port)->queue.size=value;
  (*port)->queue.size= (*port)->queue.size/16;	
  DEBUG(printf("Got notification queue size of %u bytes = %u entries\n",(*port)->queue.size*16,(*port)->queue.size);)
  (*port)->queue.rp=0;
  (*port)->queue.rp_bak=0;

  value=0;

  result=ioctl((*port)->fd, RMA2_IOCTL_GET_MAXNODEID, &value);
  if (result<0)
    {
      //perror("RMA ioctl GET_MAXNODEID failed:");
      return RMA2_ERR_ERROR;
    }
  (*port)->max_nodeid=value;
  result=ioctl((*port)->fd, RMA2_IOCTL_GET_MAXVPID, &value);
  if (result<0)
    {
      //perror("RMA ioctl GET_MAXVPID failed:");
      return RMA2_ERR_ERROR;
    }
  (*port)->max_vpid=value;

  result=ioctl((*port)->fd, RMA2_IOCTL_GET_NODEID, &value);
  if (result<0)
    {
      //perror("RMA ioctl GET_NODEID failed:");
      return RMA2_ERR_ERROR;      
    }
  (*port)->nodeid=value;
  DEBUG(printf("got a nodeid of %d from kernel\n",(*port)->nodeid);)
  result=ioctl((*port)->fd, RMA2_IOCTL_GET_VPID, &value);
  if (result<0)
    {
      //perror("RMA ioctl GET_VPID failed:");
      return RMA2_ERR_ERROR;      
    }
  (*port)->vpid=value;
  DEBUG(printf("Now mmaping noti queue\n");)
  //mmap notification queue and notification rp page...
  result=_rma2_queue_mmap(*port);
  if (result<0)
    return result;
    
  DEBUG(printf("now mmaping requester pages...\n");)
  req_mapping_size= (_extoll_page_size <= 32*4096) ? 32*4096 : _extoll_page_size*32;
  (*port)->req_pages[0]=mmap(  0,     /* preferred start */
                           req_mapping_size,        /* length in bytes */
                           PROT_WRITE,  /* protection flags */
                           MAP_SHARED,  /* mapping flags */
                           (*port)->fd, /* file descriptor */
                           0 //offset
                           );
  if ((*port)->req_pages[0]==MAP_FAILED)
    {
      fprintf(stderr,"Mmaping of requester page %d failed",0);
      
      return RMA2_ERR_MMAP;
    }
  DEBUG(printf("mapped requester page %d to virtual address %p\n", 0, (*port)->req_pages[0]);)
  for (i=1;i<32;i++)
  {
    (*port)->req_pages[i]=(*port)->req_pages[0] + (i*_extoll_page_size);
    
    DEBUG(printf("mapped requester page %d to virtual address %p\n", i, (*port)->req_pages[i]);)
  }
  
  (*port)->state_map=mmap(  0,	/* preferred start */
                           _extoll_page_size,	/* length in bytes */
                           PROT_WRITE,	/* protection flags */
                           MAP_SHARED,	/* mapping flags */
                           (*port)->fd,	/* file descriptor */
                           req_mapping_size //offset
                           );
    if ((*port)->state_map==MAP_FAILED)
    {
      fprintf(stderr,"Mmaping of wcbuf state map failed");
      
      return RMA2_ERR_MMAP;
    }
    *((uint64_t*)((*port)->state_map))=0l;
    //allocate buffer for 256 packets
    (*port)->packet_buffer=malloc(32*256);
    (*port)->committed_count=0;
    (*port)->packet_wp=0;
    (*port)->retried_packet_count=0;
    (*port)->replay_mode = RMA2_REPLAY_ALL;

  DEBUG(printf("Allocating descriptor pool\n");)
  (*port)->descriptor_queue=extoll2_construct_list(sizeof(RMA2_Descriptor), 32,32,"RMA2 Descriptor Queue"); 
  (*port)->region_queue=extoll2_construct_list(sizeof(RMA2_Region), 32,32,"RMA2 Region Queue"); 

  DEBUG(printf("Done opening\n");)
  return RMA2_SUCCESS;
}
#pragma GCC diagnostic pop

static inline void _rma2_replay_buffer_drain(RMA2_Port port, int send);
RMA2_ERROR rma2_close(RMA2_Port port)
{
  int result;
  //int i;
  uint64_t req_mapping_size;
 
  //before doing anything, send all outstanding messages
  _rma2_replay_buffer_drain(port, 0); 
  DEBUG(printf("RMA: retry count for vpid %d is %ld\n",port->vpid,  port->retried_packet_count);)
    
  req_mapping_size= (_extoll_page_size <= 32*4096) ? 32*4096 : _extoll_page_size;
  _rma2_queue_munmap(port);
    
  munmap((void*)port->req_pages[0],req_mapping_size );
  munmap((void*)port->state_map,_extoll_page_size);
  free(port->packet_buffer);
  extoll2_destruct_list(port->descriptor_queue);
  extoll2_destruct_list(port->region_queue);
  result=close(port->fd);
  if (result<0)
    {
      //perror("Closing of the RMA endpoint");
      return RMA2_ERR_ERROR;
    }
  port->fd=0;
  //   stt_free(port->STT); 
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_connect(RMA2_Port hport,RMA2_Nodeid  dest_node, RMA2_VPID dest_vpid ,RMA2_Connection_Options options, RMA2_Handle* handle)
{
  RMA2_Connection* c;
//return RMA2_SUCCESS;
  c=malloc(sizeof(RMA2_Connection));
  *handle=c;
  (*handle)->nodeid=dest_node;
  (*handle)->vpid=dest_vpid;
  
  if (options>31)
  {
    return RMA2_ERR_INV_VALUE;
  }
  
  (*handle)->req_page=hport->req_pages[options];
  
  DEBUG(printf("librma2: using requester page number %d (from options), address is %p\n",options,(*handle)->req_page);)

  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_disconnect(RMA2_Port port, RMA2_Handle handle)
{
  (void)handle;
  //send all outstanding messages before disconnecting
  if(port->replay_mode & RMA2_REPLAY_CLOSE) {
    _rma2_replay_buffer_drain(port, 0); 
  }
  return RMA2_SUCCESS;
}

RMA2_Nodeid rma2_handle_get_node(RMA2_Handle handle)
{
  //return 0;
  return handle->nodeid;
}

RMA2_VPID rma2_handle_get_vpid(RMA2_Handle handle)
{
 //return 0;
  return handle->vpid;
}


#ifdef MICEXTOLL
#define WCB_CMASK 0xf
#define WCB_RETRY_MASK 0xf000000000000000l
#define WCB_STATE_MASK 0xf000000100000000l
#else
#define WCB_CMASK 0xff
#define WCB_RETRY_MASK 0xff00000000000000l
#define WCB_STATE_MASK 0xff00000100000000l
#endif

static inline void _rma2_replay_buffer(RMA2_Port port)
{
  uint32_t start;
  uint32_t stop;
  unsigned int i,j;
  uint64_t* msg;
  //int count=0;
  uint64_t retry;
  uint32_t retry_count;
  uint64_t assembly,streamout;
  uint64_t state;
  volatile uint64_t * requester_page;
  
//   sleep(1);
  //buffer is full, need to update commited count and/or check retry
  //printf("_rma2_replay_buffer called\n");
  state=*(port->state_map);
  //printf("read state 0x%lx from hw\n",state);
  retry=state & 0x100000000l;
  port->committed_count=state & WCB_CMASK;
  //FIXME:
  //make sure that committed count + retry count = expected count (which is what?)
  //printf("updated commited count to 0x%x, packet_wp is 0x%x\n",port->committed_count, port->packet_wp);
  retry_count=(state & WCB_RETRY_MASK) >> 56l;

  //update commited count
  //printf("retry is %ld\n",retry);
  if (retry!=0l && retry_count!=0) //oops, a retry occured, need to play back!
  {
	  DEBUG(printf("retry is set!\n");)
	   //first make sure that the buffer for this vpid is idle: bit 33==bit34
	   assembly =(state & 0x200000000l) >> 33l;
	   streamout=(state & 0x400000000l) >> 34l;

	   while (assembly!=streamout) {
	     state=*(port->state_map);
	     assembly =(state & 0x200000000l) >> 33l;
	     streamout=(state & 0x400000000l) >> 34l;
	     DEBUG(printf("idle? state %lx assembly=%ld streamout=%ld\n",state,assembly,streamout);)
	     //assert(assembly==streamout);
	   } 
	   DEBUG(printf("ok, idle\n");)
	    while (((port->packet_wp-port->committed_count) & WCB_CMASK) != retry_count)
	    {
	      state=*(port->state_map);
	      port->committed_count=state & WCB_CMASK;
	      retry_count=(state & WCB_RETRY_MASK) >> 56l;
	    }
	    //reset the retry bit!
	   DEBUG(printf("write back state of %lx, was %lx\n",state & ~0xff00000100000000l,state);)
	    //abort();
	    *(port->state_map)=state & ~(0xff00000100000000l);
	    
	    do {
	      retry_count=((*(port->state_map))& (WCB_RETRY_MASK)) >> 56l;
	      
	      if (retry_count!=0) {
		*(port->state_map)=state & ~(WCB_STATE_MASK);
	      }
	    } while (retry_count!=0);
	    
	    //start over
	    start=(port->committed_count) & WCB_CMASK;
	    stop=port->packet_wp;
	    DEBUG(printf("Retry called: start %d stop %d retry_count %d\n",start,stop,retry_count);)
    
    
	    for (i=start;i!=stop;i=(i+1) & WCB_CMASK) //iterate over all messages in buffer
	    {
	      DEBUG(printf("replay message idx %d\n",i);	      )
	      //Message are always 192 bits, or 3 qwords. 4th quadword holds the requester page used
	      
	      msg=&(port->packet_buffer[i<<2]);
	      DEBUG(printf("Message saved at address %p, used index is %u\n",msg,i<<2);)
	      requester_page=(volatile uint64_t*)msg[3];
	      
	      for (j=0;j<3;j++)
	      {
		DEBUG(printf("copy word %d\n",j);)
		requester_page[j]=msg[j];
	      }
	      DEBUG(printf("copy done\n");)
	      flush_wc();
	      port->retried_packet_count++;
	      //count++;
	    }
  }

  return;
}

static inline void _rma2_replay_buffer_drain(RMA2_Port port, const int send)
{
  //TODO:is there a better way to express the code for the send part and the drain part?!
  uint32_t wp = port->packet_wp;
  if(send) {
    //check if packet buffer is full
    while( ((wp+1) & WCB_CMASK) == port->committed_count) {
      DEBUG(printf("port wp is %d, committed count is %x, replay_buffer!\n",wp, port->committed_count);)
        _rma2_replay_buffer(port);
    }
  } else {
    while( (wp & WCB_CMASK) != port->committed_count) {
      DEBUG(printf("port wp is %d, committed count is %x, replay_buffer!\n",wp, port->committed_count);)
        _rma2_replay_buffer(port);
    }
  }
}

void rma2_replay_buffer_drain(RMA2_Port port)
{
  _rma2_replay_buffer_drain(port, 0);
}

void rma2_set_replay_mode(RMA2_Port port, RMA2_Replay_Buffer_Mode mode)
{
  port->replay_mode = mode;
}

RMA2_Replay_Buffer_Mode rma2_get_replay_buffer_mode(RMA2_Port port)
{
  return port->replay_mode;
}



RMA2_ERROR rma2_post_put_bt(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
			     RMA2_Command_Modifier modifier)
{
  RMA2_NLA src;
  rma2_get_nla(src_region, src_offset, &src);
  DEBUG(printf("Computed Source address is: %lx\n",src);)
  assert(size!=0);
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_BT_PUT, size, spec,
			modifier,src,dest_address);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_put_bt_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA src, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
				   RMA2_Command_Modifier modifier)
{ 
  DEBUG(printf("Computed Source address is: %lx\n",src);)
    assert(size!=0);
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_BT_PUT, size, spec,
			modifier,src,dest_address);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_put_qw(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
			    RMA2_Command_Modifier modifier)
{
  RMA2_NLA src;
  if ( (size & 0x7)!=0) return RMA2_ERR_WRONG_ARG;
    assert(size!=0);
  rma2_get_nla(src_region, src_offset, &src);
  DEBUG(printf("Computed Source address is: %lx\n",src);)
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_QW_PUT, size, spec,
			modifier,src,dest_address);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
 
}

RMA2_ERROR rma2_post_put_qw_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA src, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
				   RMA2_Command_Modifier modifier)
{
  if ( (size & 0x7)!=0) return RMA2_ERR_WRONG_ARG;
    assert(size!=0);
  DEBUG(printf("Computed Source address is: %lx\n",src);)
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_QW_PUT, size, spec,
			modifier,src,dest_address);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}
  
RMA2_ERROR rma2_post_get_bt(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
			    RMA2_Command_Modifier modifier)
{
  RMA2_NLA src;
  rma2_get_nla(src_region, src_offset, &src);
    assert(size!=0);
  DEBUG(printf("Computed Source address is: %lx\n",src);)
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_BT_GET, size, spec,
			modifier,dest_address,src);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_get_bt_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA  src, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
				   RMA2_Command_Modifier modifier)
{ 
    assert(size!=0);
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_BT_GET, size, spec,
			modifier,dest_address,src);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_get_qw(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
			    RMA2_Command_Modifier modifier)
{
  RMA2_NLA src;
  if ( (size & 0x7)!=0) return RMA2_ERR_WRONG_ARG;
  rma2_get_nla(src_region, src_offset, &src);
  DEBUG(printf("Computed Source address is: %lx\n",src);)
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_QW_GET, size, spec,
			modifier,dest_address,src);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_get_qw_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA  src, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
				   RMA2_Command_Modifier modifier)
{ 
    assert(size!=0);
  if ( (size & 0x7)!=0) return RMA2_ERR_WRONG_ARG;
  rma2_desc_set_put_get(&__static_desc,handle,RMA2_QW_GET, size, spec,
			modifier,dest_address,src);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_lock(RMA2_Port port,RMA2_Handle handle, uint32_t  target,  uint32_t lock_number, int32_t cmp_value, int32_t add_value, RMA2_Notification_Spec spec,
			  RMA2_Command_Modifier modifier)
{
  rma2_desc_set_lock(&__static_desc, handle, spec, modifier, target, lock_number, cmp_value, add_value);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_notification(RMA2_Port port,RMA2_Handle handle, RMA2_Class class, uint64_t value, RMA2_Notification_Spec spec,
				  RMA2_Command_Modifier modifier)
{
  rma2_desc_set_noti_put(&__static_desc, handle, spec,modifier, class,value);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_immediate_put(RMA2_Port port,RMA2_Handle handle, uint32_t count, uint64_t value,  RMA2_NLA dest_address, RMA2_Notification_Spec spec,
				   RMA2_Command_Modifier modifier)
{ 
    assert(count!=0);
  rma2_desc_set_immediate_put(&__static_desc, handle, spec,modifier, count, dest_address, value);
  rma2_post_descriptor(port, handle, &__static_desc);
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_post_descriptor(RMA2_Port port,RMA2_Handle handle, RMA2_Descriptor* desc)
{ 
  int i;
#ifdef PACKET_BUFFER
  uint32_t wp=port->packet_wp;
  uint32_t waddr=wp<<2;
#endif
  
  DEBUG(printf("post %lx %lx %lx handle\n",desc->value[0], desc->value[1],desc->value[2]);)
  DEBUG(printf("writing to address %p\n",handle->req_page);)
  //abort();
#ifdef PACKET_BUFFER
  if(port->replay_mode & RMA2_REPLAY_POST) {
    _rma2_replay_buffer_drain(port, 1);
  }
#endif
  
  for (i=0;i<3;i++)
  {
    handle->req_page[i]=desc->value[i];
  }
  flush_wc();	//just send it
#ifdef PACKET_BUFFER  
  //record descriptor
  for (i=0;i<3;i++)
  {
    DEBUG(printf("recording %lx to address %p\n",desc->value[i],&(port->packet_buffer[waddr+i]));)
    port->packet_buffer[waddr+i]=desc->value[i];
  }  
  //record requester page used
  DEBUG(printf("recording %p to address %p\n",handle->req_page,&(port->packet_buffer[waddr+3]));)
  port->packet_buffer[waddr+3]=(uint64_t)handle->req_page;
  //inc packet buffer wp
  port->packet_wp=(wp+1) & WCB_CMASK;
  DEBUG(printf("update packet_wp to  %x\n", port->packet_wp);)
#endif
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_desc_alloc(RMA2_Port port, RMA2_Descriptor** desc)
{ 
  *desc=(RMA2_Descriptor*)extoll2_new_item(port->descriptor_queue);
  if (*desc==0) return RMA2_ERR_NO_DESC_MEM;
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_desc_free(RMA2_Port port, RMA2_Descriptor* desc)
{
 extoll2_free_item(port->descriptor_queue,(extoll_list_head_t* )desc);
 return RMA2_SUCCESS;
}

RMA2_ERROR rma2_desc_query_free(RMA2_Port port, int* num)
{
  *num=extoll2_list_get_freelist_size(port->descriptor_queue);  
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_desc_set_put_get(RMA2_Descriptor* desc, RMA2_Handle handle,RMA2_Command command, uint32_t count, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, uint64_t read_address, uint64_t write_address)
{  
  desc->handle=handle;
  desc->value[0]=(((count-1) & 0x7fffffl)<<40l) | ((modifier & 0xfl)<<35l)| ((noti & 0x7l)<<32l)  | 
	       ((handle->nodeid & 0xffffl)<<16l) | ((handle->vpid & 0xffl)<<8l) | ((command & 0xfl)<<2l) | 3l;
  desc->value[1]=read_address;
  desc->value[2]=write_address;
  
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_desc_set_immediate_put(RMA2_Descriptor* desc, RMA2_Handle handle, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, uint32_t count, uint64_t write_address, uint64_t value)
{  
  desc->handle=handle;
  desc->value[0]=(( (count-1) & 0x7l)<<40l) | ((modifier & 0xfl)<<35l)| ((noti & 0x7l)<<32l)  | 
	       ((handle->nodeid & 0xffffl)<<16l) | ((handle->vpid & 0xffl)<<8l) | ((RMA2_IMMEDIATE_PUT)<<2l) | 3l;
  desc->value[1]=write_address;
  desc->value[2]=value;
  
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_desc_set_noti_put(RMA2_Descriptor* desc, RMA2_Handle handle, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, uint8_t class, uint64_t value)
{  
  desc->handle=handle;
  desc->value[0]=((class & 0xffl)<<40l) | ((modifier & 0xfl)<<35l)| ((noti & 0x7l)<<32l)  | 
	       ((handle->nodeid & 0xffffl)<<16l) | ((handle->vpid & 0xffl)<<8l) | ((RMA2_NOTIFICATION_PUT)<<2l) | 3l;
  desc->value[1]=value;
  desc->value[2]=0l;
  
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_desc_set_lock(RMA2_Descriptor* desc, RMA2_Handle handle, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, uint8_t type, uint32_t lock_number, uint32_t compare, uint32_t add_value)
{  
  desc->handle=handle;
  DEBUG(printf("number: %u compare %u add %u\n",lock_number,compare,add_value);)
  if (type==0)
    desc->value[0]=((modifier & 0xfl)<<35l)| ((noti & 0x7l)<<32l)  | 
	       ((handle->nodeid & 0xffffl)<<16l) | ((handle->vpid & 0xffl)<<8l) | ((RMA2_LOCK_REQ)<<2l) | 3l;
  else
    desc->value[0]= (1l<<40l) | ((modifier & 0xfl)<<35l)| ((noti & 0x7l)<<32l)  | 
	       ((handle->nodeid & 0xffffl)<<16l) | ((handle->vpid & 0xffl)<<8l) | ((RMA2_LOCK_REQ)<<2l) | 3l;
    
  //desc->value[1]=(<<32l) | (compare & 0xffffffffl);
  desc->value[1]=((compare & 0xffffffffl)<<32l) |(lock_number & 0xffffffl);
  desc->value[2]=(add_value & 0xffffffffl)<<32l;
  
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_noti_probe(RMA2_Port port, RMA2_Notification** notification)
{
  if (port->queue.queue[port->queue.rp].word1.value!=0) //standard queue is not empty
    {
      //DEBUG(printf("probe %d %d %d\n", list_empty(&port->extension.used_list), port->queue.queue[port->queue.rp].command_field, port->queue.rp);)
      *notification=&port->queue.queue[port->queue.rp];  
      return RMA2_SUCCESS;
      //      SKIP_ENTRIES(port->queue.queue, port->queue.rp,port->queue.wp);
    }
#ifdef PACKET_BUFFER        
  else
  {
      if(port->replay_mode & RMA2_REPLAY_NOTI) {
        _rma2_replay_buffer_drain(port,0);
      }
  }
#endif        
  return RMA2_NO_NOTI;
}

RMA2_ERROR rma2_noti_get_block(RMA2_Port port, RMA2_Notification** notification)
{
  volatile uint64_t* status;
#ifdef PACKET_BUFFER
   uint64_t i=0;
#endif   
 
   DEBUG(printf("Wait for notification at offset %u\n", port->queue.rp);)
  status=&(port->queue.queue[port->queue.rp].word1.value);
 /*  printf("prior words: %x , current word: %x next: %x\n", */
/*          &(port->queue.queue[(rp-1 % 0xff)].group_error), &(port->queue.queue[rp].group_error), &(port->queue.queue[(rp+1) % 0xff].group_error)); */
  //printf("status is %lx, nextword is %lx\n", *status, *(status+1));
  while ( *status==0) {
#ifdef PACKET_BUFFER
     i++;
   /* busy wait*/ 
    if ((i & 0x1000l)!=0l)
    {      
      if(port->replay_mode & RMA2_REPLAY_NOTI) {
        _rma2_replay_buffer_drain(port,0);
      }
    }
#endif    
  } //wait for noti...
  DEBUG(printf("Found noti...\n");)
  *notification=&port->queue.queue[port->queue.rp];      
  return RMA2_SUCCESS;
}



RMA2_ERROR __rma2_noti_match_choice(RMA2_Port port, RMA2_Class class, RMA2_Command cmd, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification_Spec spec, RMA2_NLA nla,
								  RMA2_Notification** notification, int flags)
{
  int match;
  uint32_t offset;
  RMA2_Command n_cmd;
  RMA2_ERROR rc;

  if ((flags & RMA2_BLOCK_FLAG) ==1)
  {
    rc=rma2_noti_get_block(port, notification);
    if (rc!=RMA2_SUCCESS) return rc;
  }
  else
  { 
    rc=rma2_noti_probe(port, notification);
   if (rc!=RMA2_SUCCESS) return rc;
  }

  DEBUG(printf("matching...\n");)
  match=0;     
  if (spec == rma2_noti_get_notification_type(*notification)) match++;
  DEBUG(printf("match1 %d\n",match);)
  n_cmd=rma2_noti_get_cmd(*notification);
  if ( (cmd==RMA2_ANY) || (n_cmd== cmd) || 
	( (cmd == RMA2_GETS) && ( (n_cmd==RMA2_BT_GET) || (n_cmd==RMA2_QW_GET) )) ||
	( (cmd == RMA2_PUTS) && ( (n_cmd==RMA2_BT_PUT) || (n_cmd==RMA2_QW_PUT) || (n_cmd==RMA2_QW_PUT) || (n_cmd==RMA2_IMMEDIATE_PUT) )) )
      match++;
  DEBUG(printf("match2 %d\n",match);)
  if ( (vpid==RMA2_VPID_ANY)   || (rma2_noti_get_remote_vpid(*notification)==vpid)    ) match++;
  DEBUG(printf("match3 %d\n",match);)
  if ( (vpid==RMA2_NODEID_ANY) || (rma2_noti_get_remote_nodeid(*notification)==nodeid)  ) match++;
  DEBUG(printf("match4 %d\n",match);)
  if ( (class==RMA2_CLASS_ANY) || (rma2_noti_get_notiput_class(*notification)==class) ) match++;
  DEBUG(printf("match5 %d\n",match);)

  if ( (flags & RMA2_MATCH_LAST_NLA_FLAG) > 0 )
      {
	    offset = rma2_noti_get_size(*notification);// ((*notification)->count + 1)  * ( ( ((*notification)->command>>5) == RMA2_CL_GET || ((*notification)->command>>5) == RMA2_CL_PUT) ?  64 : 1);
      }
      else
	    offset = 0;
  DEBUG(printf("nla match: nla is %lx, nla-offset is %lx, noti address is %lx\n",nla,nla-offset,rma2_noti_get_local_address(*notification));)
  if ( (nla==RMA2_ANY_NLA) || ((nla-offset) == rma2_noti_get_local_address(*notification))) match++; //note that this also works for noti's with destination_nla as this is a union
  DEBUG(printf("match6 %d\n",match);)
  DEBUG(printf("match: %d %d %d %d %d %d %d %d %d %d \n", spec, rma2_noti_get_notification_type(*notification),rma2_noti_get_cmd(*notification), 
		      cmd,rma2_noti_get_remote_vpid(*notification), vpid,rma2_noti_get_remote_nodeid(*notification), nodeid, match,class);)
  if (match==6) //all criteria matched... 
    {
      DEBUG(printf("%d match %p\n", port->nodeid, itemp);)
      return RMA2_SUCCESS;
    }
    DEBUG(else
      printf("match only: %d\n",match);)
  
  return RMA2_ERR_NO_MATCH;
}

RMA2_ERROR __rma2_noti_lock_match_choice(RMA2_Port port, uint32_t lock,  RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification, int block)
{
  (void)nodeid;
  int match;
   RMA2_ERROR rc;

  if (block==1)
  {
    rc=rma2_noti_get_block(port, notification);
    if (rc!=RMA2_SUCCESS) return rc;
  }
  else
  { 
    rc=rma2_noti_probe(port, notification);
   if (rc!=RMA2_SUCCESS) return rc;
  }
  
  match=0;

  if ( (RMA2_RESPONDER_NOTIFICATION & rma2_noti_get_notification_type(*notification)) !=0) match++;      
  //plus one ensures that value can not be zero.... ->32 bit ints are by good margin larger than fields.
  if ( (RMA2_LOCK_REQ ==rma2_noti_get_cmd(*notification)) || (RMA2_LOCK_RSP ==rma2_noti_get_cmd(*notification)) ) match++;
  if ( (vpid==RMA2_VPID_ANY)   || (rma2_noti_get_remote_vpid(*notification)==vpid)    ) match++;
  if ( (vpid==RMA2_NODEID_ANY) || (rma2_noti_get_remote_nodeid(*notification)==vpid)  ) match++;
  if ( lock==rma2_noti_get_lock_number(*notification) ) match++;
                     
  if (match==5) //all criteria matched...
  {
     return RMA2_SUCCESS;
  }
 
  return RMA2_ERR_NO_MATCH;
}

RMA2_ERROR rma2_noti_match(RMA2_Port port, RMA2_Command cmd, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification_Spec type,RMA2_NLA nla, int flags, RMA2_Notification** notification)
{
  //a class of 0xff is transformed to 0 and anded -> not used...
  return __rma2_noti_match_choice(port,0xff, cmd, nodeid, vpid,type,nla, notification,flags);
}

RMA2_ERROR rma2_noti_match_block(RMA2_Port port, RMA2_Command cmd, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification_Spec type, RMA2_NLA nla,int flags, RMA2_Notification** notification)
{
  //a class of 0xff is transformed to 0 and anded -> not used...
  return   __rma2_noti_match_choice(port, 0xff, cmd, nodeid, vpid,type,nla, notification,flags | RMA2_BLOCK_FLAG);
}

RMA2_ERROR rma2_noti_noti_match(RMA2_Port port, RMA2_Class class, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification)
{
  //command and notification spec are fixed in this case
  return   __rma2_noti_match_choice(port,class, RMA2_NOTIFICATION_PUT, nodeid, vpid,RMA2_COMPLETER_NOTIFICATION,RMA2_ANY_NLA,notification,0);
}

RMA2_ERROR rma2_noti_noti_match_block(RMA2_Port port, RMA2_Class class, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification)
{
  //command and notification spec are fixed in this case
  return   __rma2_noti_match_choice(port,class, RMA2_NOTIFICATION_PUT, nodeid, vpid,RMA2_COMPLETER_NOTIFICATION,RMA2_ANY_NLA,notification,RMA2_BLOCK_FLAG);
}

RMA2_ERROR rma2_noti_lock_match(RMA2_Port port, uint32_t lockid, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification)
{
  return   __rma2_noti_lock_match_choice(port,lockid,nodeid, vpid, notification, 0);
}

RMA2_ERROR rma2_noti_lock_match_block(RMA2_Port port, uint32_t lockid, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification)
{
  return   __rma2_noti_lock_match_choice(port,lockid,nodeid, vpid, notification, 1);
}

RMA2_ERROR rma2_noti_dup(RMA2_Notification* input, RMA2_Notification* output)
{
  memcpy(output,input,sizeof(RMA2_Notification));
  return RMA2_SUCCESS;
}

  
void rma2_noti_dump(RMA2_Notification* noti)
{
  printf("Notification raw values: %lx %lx\n",noti->word1.value, noti->word0.value);
  printf("Command: %x Notification Type (Spec): %x Mode bits: %x Error: %x\n",
      rma2_noti_get_cmd(noti), rma2_noti_get_notification_type(noti), rma2_noti_get_mode(noti), rma2_noti_get_error(noti));
  printf("Remote nodeid is %x remote vpid is %x\n", rma2_noti_get_remote_nodeid(noti), rma2_noti_get_remote_vpid(noti));
  if (rma2_noti_get_cmd(noti)==RMA2_NOTIFICATION_PUT)
  {
    printf("Notification put payload: %lx class: %x\n", rma2_noti_get_notiput_payload(noti), rma2_noti_get_notiput_class(noti));
  }
  else if (rma2_noti_get_cmd(noti)==RMA2_LOCK_REQ || rma2_noti_get_cmd(noti)==RMA2_LOCK_RSP)
  {
    printf("Lock value %x lock number %x lock result %x\n", rma2_noti_get_lock_value(noti), rma2_noti_get_lock_number(noti), rma2_noti_get_lock_result(noti));
  }
  else {
    printf("Sized put/get. Local address is %lx payload size is %d bytes (decimal)\n", rma2_noti_get_local_address(noti), rma2_noti_get_size(noti));
  }
return;
}

void rma2_noti_fdump(RMA2_Notification* noti, FILE * filedescriptor)
{
  fprintf(filedescriptor,"Notification raw values: %lx %lx\n",noti->word1.value, noti->word0.value);
  fprintf(filedescriptor,"Command: %x Notification Type (Spec): %x Mode bits: %x Error: %x\n",
      rma2_noti_get_cmd(noti), rma2_noti_get_notification_type(noti), rma2_noti_get_mode(noti), rma2_noti_get_error(noti));
  fprintf(filedescriptor,"Remote nodeid is %x remote vpid is %x\n", rma2_noti_get_remote_nodeid(noti), rma2_noti_get_remote_vpid(noti));
  if (rma2_noti_get_cmd(noti)==RMA2_NOTIFICATION_PUT)
  {
    fprintf(filedescriptor,"Notification put payload: %lx class: %x\n", rma2_noti_get_notiput_payload(noti), rma2_noti_get_notiput_class(noti));
  }
  else if (rma2_noti_get_cmd(noti)==RMA2_LOCK_REQ || rma2_noti_get_cmd(noti)==RMA2_LOCK_RSP)
  {
    fprintf(filedescriptor,"Lock value %x lock number %x lock result %x\n", rma2_noti_get_lock_value(noti), rma2_noti_get_lock_number(noti), rma2_noti_get_lock_result(noti));
  }
  else {
    fprintf(filedescriptor,"Sized put/get. Local address is %lx payload size is %d bytes (decimal)\n", rma2_noti_get_local_address(noti), rma2_noti_get_size(noti));
  }
return;
}

void rma2_noti_sdump(RMA2_Notification* noti, char* pstring, int size)
{
  char string[128];
  
  snprintf(string,128,"Notification raw values: %lx %lx\n",noti->word1.value, noti->word0.value);
  strncat(pstring,string,size);
  snprintf(string,128,"Command: %x Notification Type (Spec): %x Mode bits: %x Error: %x\n",
      rma2_noti_get_cmd(noti), rma2_noti_get_notification_type(noti), rma2_noti_get_mode(noti), rma2_noti_get_error(noti));
  strncat(pstring,string,size);
  snprintf(string,128,"Remote nodeid is %x remote vpid is %x\n", rma2_noti_get_remote_nodeid(noti), rma2_noti_get_remote_vpid(noti));
  strncat(pstring,string,size);
  if (rma2_noti_get_cmd(noti)==RMA2_NOTIFICATION_PUT)
  {
    snprintf(string,128,"Notification put payload: %lx class: %x\n", rma2_noti_get_notiput_payload(noti), rma2_noti_get_notiput_class(noti));
    strncat(pstring,string,size);
  }
  else if (rma2_noti_get_cmd(noti)==RMA2_LOCK_REQ || rma2_noti_get_cmd(noti)==RMA2_LOCK_RSP)
  {
    snprintf(string,128,"Lock value %x lock number %x lock result %x\n", rma2_noti_get_lock_value(noti), rma2_noti_get_lock_number(noti), rma2_noti_get_lock_result(noti));
    strncat(pstring,string,size);
  }
  else {
    snprintf(string,128,"Sized put/get. Local address is %lx payload size is %d bytes (decimal)\n", rma2_noti_get_local_address(noti), rma2_noti_get_size(noti));
    strncat(pstring,string,size);
  }
return;
}

RMA2_Command rma2_noti_get_cmd(RMA2_Notification* noti)
{
  //printf("get command: %lx %x\n",(noti->word1.value >> 51l) & 0xfl,(RMA2_Command)((noti->word1.value >> 51l) & 0xfl));
  return (RMA2_Command)((noti->word1.value >> 51l) & 0xfl);
}

RMA2_Notification_Spec rma2_noti_get_notification_type(RMA2_Notification* noti)
{
  return (noti->word1.value >> 48l) & 0x7l;
}

RMA2_Notification_Modifier rma2_noti_get_mode(RMA2_Notification *noti)
{
  return (noti->word1.value >> 55l) & 0x3fl;
}

uint8_t rma2_noti_get_error(RMA2_Notification* noti)
{
   return (noti->word1.value >> 61l) & 0x7l;
}

RMA2_Nodeid rma2_noti_get_remote_nodeid(RMA2_Notification* noti)
{
   return (noti->word1.value >>32l) & 0xffffl;
}

RMA2_VPID rma2_noti_get_remote_vpid(RMA2_Notification* noti)
{
  return (noti->word1.value >> 23l ) & 0xffl;
}

uint32_t rma2_noti_get_size(RMA2_Notification* noti)
{
  return ((noti->word1.value) & 0x7fffffl) +1;
}

uint32_t rma2_noti_get_total_size(RMA2_NLA start_nla, RMA2_Notification *noti)
{
  return rma2_noti_get_local_address(noti)-start_nla+rma2_noti_get_size(noti);
}


uint64_t rma2_noti_get_local_address(RMA2_Notification* noti)
{
  return (noti->word0.value);
}

uint64_t rma2_noti_get_notiput_payload(RMA2_Notification* noti)
{
  return noti->word0.value;
}

uint8_t rma2_noti_get_notiput_class(RMA2_Notification* noti)
{
  return noti->word1.value & 0xffl;
}

uint32_t rma2_noti_get_lock_value(RMA2_Notification* noti)
{
  return (noti->word0.value >>32l) & 0xffffffffl;
}

uint32_t rma2_noti_get_lock_number(RMA2_Notification* noti)
{
  return (noti->word0.value ) & 0xffffffl;
}

uint8_t rma2_noti_get_lock_result(RMA2_Notification* noti)
{
  return (noti->word0.value >>24l) & 0xffl;
}


RMA2_ERROR rma2_noti_free(RMA2_Port port, RMA2_Notification* notification)
{ 
  uint64_t idx = notification - port->queue.queue;
  DEBUG(printf("Free nqueue noti at rp %lu\n", idx);)
  if (idx!=port->queue.rp)
    {
      fprintf(stderr,"Oops, Notification queue out-of-order consumation problem: Software bug at line %u in file %s!\n",__LINE__,__FILE__);
      abort();
    }
  port->queue.queue[idx].word1.value=0;
  //port->queue.queue[idx].group_error=0;
  INKRP(port->queue.rp, port->queue.size);
  UPDATE_RP(port->queue.rp_bak, port->queue.rp, port->queue.rp_wb, port->queue.size);
  DEBUG(printf("Noti read pointer are: rp %u rp_bak: %u\n", port->queue.rp_bak,port->queue.rp);)

  return RMA2_SUCCESS;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
RMA2_ERROR rma2_register(RMA2_Port port, void* address, size_t size, RMA2_Region** r)
{ //return RMA2_SUCCESS;
#ifndef ENABLE_REGIONMANAGER
  int result;
  
  RMA2_Region* region=(RMA2_Region*)extoll2_new_item(port->region_queue);
  //RMA2_Region* region=malloc(sizeof(RMA2_Region));
  if (region==0)
  {
	 perror("rma2_register malloc:");
	 return RMA2_ERR_ERROR;
  }

  DEBUG(printf("registering from %p with size %lu\n",address, size);)
  region->start=(void*)(( (unsigned long) address) & 0xfffffffffffff000l);
  //region->end=(void*)(( (unsigned long) address+size) & 0xfffffffffffff000l);
  region->nla=0l;    
  //region->overlap=0;
  //region->overlap_idx=0;
  region->offset = ((unsigned long) address) & 0xfffl;
  size = size + region->offset; //add unaligned offset to size
  region->count = size>>12l;
  if ( (size & 0xfffl)!=0l) region->count++;
  //assert( (((size-1)>>12)+1) == region->count);
/*  if (( ((unsigned long) address) & 0xfffl)!=0) //could add one page too much!
  {
  // printf("unaligned start address2..\n");
   region->count++;
  }*/
  region->size=region->count*4096l;
 
  region->end=region->start+region->count*4096l; //calculate the size that is actually registered with the system after the ioctl  
  //region->refcount.global=0l;
  //region->child=0;
  
  DEBUG(printf("aligned values: start  %p, end %p, size %lu, count %lu\n",region->start, region->end, region->size, region->count);)
  
  //printf("direct register: offset is %x\n", region->offset);     
  result=ioctl(port->fd, RMA2_IOCTL_REGISTER_REGION, region);
  if (result<0)
    {
      perror("RMA ioctl REGISTER_REGION failed:");
      return RMA2_ERR_ERROR;
    }
  region->endpoint=port;
  region->parent=0;
  *r=(RMA2_Region*)region;
  DEBUG(printf("got a nla from registering of %lx region is %p\n", (*r)->nla,region);)
  return RMA2_SUCCESS;
  #else
  return rma2_map_add_remote(port,address,size,region);  
  #endif
}

RMA2_ERROR rma2_register_cached(RMA2_Port port, void* address, size_t size, RMA2_Region** r) __attribute__ ((weak, alias ("rma2_register")));
RMA2_ERROR rma2_register_nomalloc(RMA2_Port port, void* address, size_t size, RMA2_Region* region)
{ //return RMA2_SUCCESS;
#ifndef ENABLE_REGIONMANAGER
  int result;
  //RMA2_Region* region=malloc(sizeof(RMA2_Region));
                 
  //printf("registering from %p with size %lu\n",address, size);
  region->start=(void*)(( (unsigned long) address) & 0xfffffffffffff000l);
  //region->end=(void*)(( (unsigned long) address+size) & 0xfffffffffffff000l);
  region->nla=0l;    
  //region->overlap=0;
  //region->overlap_idx=0;
  region->offset = ((unsigned long) address) & 0xfffl;
  size = size + region->offset; //add unaligned offset to size
  region->count = size>>12;
  if ( (size & 0xfffl)!=0l) region->count++;
/*  if (( ((unsigned long) address) & 0xfffl)!=0) //could add one page too much!
  {
  // printf("unaligned start address2..\n");
   region->count++;
  }*/
  region->size=region->count*4096;
 
  region->end=region->start+region->count*4096; //calculate the size that is actually registered with the system after the ioctl  
  //region->refcount.global=0l;
  //region->child=0;
  
  //printf("aligned values: start  %p, end %p, size %lu, count %lu\n",region->start, region->end, region->size, region->count);
  
  //printf("direct register: offset is %x\n", region->offset);     
  result=ioctl(port->fd, RMA2_IOCTL_REGISTER_REGION, region);
  if (result<0)
    {
      //perror("RMA ioctl REGISTER_REGION failed:");
      return RMA2_ERR_ERROR;
    }
  //*r=region;
  return RMA2_SUCCESS;
  #else
  return rma2_map_add_remote(port,address,size,region);  
  #endif
}
#pragma GCC diagnostic pop

RMA2_ERROR rma2_unregister(RMA2_Port port, RMA2_Region* region)
{ //return RMA2_SUCCESS;
 #ifndef ENABLE_REGIONMANAGER
  int result;
  assert(port);
  assert(region);
  result=ioctl(port->fd, RMA2_IOCTL_UNREGISTER_REGION, region);
  if (result<0)
    {
      //perror("RMA ioctl UNREGISTER_REGION failed:");
      return RMA2_ERR_ERROR;
    }
   //free(region);
  extoll2_free_item(port->region_queue,(extoll_list_head_t* )region);
  return RMA2_SUCCESS;
 #else
  return rma2_map_remove_remote(port,region);//region->start, region->size);
 #endif
}
RMA2_ERROR rma2_unregister_cached(RMA2_Port port, RMA2_Region* region) __attribute__ ((weak, alias ("rma2_unregister")));
RMA2_ERROR rma2_unregister_nofree(RMA2_Port port, RMA2_Region* region)
{ //return RMA2_SUCCESS;
 #ifndef ENABLE_REGIONMANAGER
  int result;
  result=ioctl(port->fd, RMA2_IOCTL_UNREGISTER_REGION, region);
  if (result<0)
    {
      //perror("RMA ioctl UNREGISTER_REGION failed:");
      return RMA2_ERR_ERROR;
    }
   //free(region);
  return RMA2_SUCCESS;
 #else
  return rma2_map_remove_remote(port,region);//region->start, region->size);
 #endif
}

// RMA2_ERROR rma2_register_adhoc(RMA2_Port port, void* address,  RMA2_Region** region)
// { return RMA2_SUCCESS;
// #ifndef ENABLE_REGIONMANAGER
//   return rma2_register(port,address, 4096, region);
// #else
//   return rma2_map_add_local(port,address,4096,region);
// #endif
// }
// 
// RMA2_ERROR rma2_unregister_adhoc(RMA2_Port port, RMA2_Region* region)
// {
// #ifndef ENABLE_REGIONMANAGER
//   return rma2_unregister(port,region);
// #else
//   return rma2_map_remove_local(port,region);//region->start, region->size);
// #endif
// }

RMA2_ERROR rma2_get_nla(RMA2_Region* region, size_t offset, RMA2_NLA* nla)
{ //return RMA2_SUCCESS;
  assert(region!=0);
  assert(nla!=0);
  
  assert((uint64_t)region>0x1000);
  assert((uint64_t)nla>0x1000);
  
  //#define DEBUG(a) a
  DEBUG(printf("get nla...\n");)
  offset+=region->offset;// also take base region offset into account...
  DEBUG(printf("offset is: %lx\n", offset);)
    //*nla=region->nlas[offset>>12] +(offset & 0xfff);
  *nla=region->nla+offset;
  DEBUG(printf("nla is %lx region is %p\n", *nla, region);)
  return RMA2_SUCCESS;
  //#define DEBUG(a) 
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
int rma2_get_region_contained(RMA2_Region* region, void* start, size_t size)
{
   if (region->start <= start && region->end >= start+size)
   {
    DEBUG(printf("existing region starts at %p , requested at %p OFFSET=%lu size of existing region %ld size of request %ld offset plus size of request %ld\n",
            region->start,start,start-region->start,region->end-region->start, size, start-region->start+size);)
    return 1;
   }
   else return 0;
}

int rma2_get_region_overlap(RMA2_Region* region, void* start, size_t size)
{
    if ( (region->start>=start && region->start<=start+size) ||
         (region->end>=start && region->end<=start+size) ||
         (region->start <= start && region->end >= start+size) )
  
//     if (  (start>=region->start && start<region->end ) || //region overlaps at the beginning of memory
//           (start+size>=region->start && start+size<region->end) || //region overlaps at the end of memory
//           (start<=region->start && start+size>=region->end) ) //region is within memory
   {
    DEBUG(printf("existing region starts at %p , requested at %p OFFSET=%lu size of existing region %ld size of request %ld offset plus size of request %ld\n",
            region->start,start,start-region->start,region->end-region->start, size, start-region->start+size);)
    return 1;
  }
  else return 0;
}

RMA2_ERROR rma2_get_va(RMA2_Region* region, size_t offset, void** va)
{ 
   *va=region->start+offset+region->offset;
   return RMA2_SUCCESS;
}
#pragma GCC diagnostic pop

int rma2_get_region_equal(RMA2_Region* region, void* start, size_t size)
{
  if (region->start == start && region->size == size) return 1;
  else return 0;
}

RMA2_ERROR rma2_get_size(RMA2_Region* region, size_t* size)
{ //return RMA2_SUCCESS;
  *size=region->size;
  return RMA2_SUCCESS;
}

RMA2_ERROR rma2_get_page_count(RMA2_Region* region, size_t* count)
{ //return RMA2_SUCCESS;
  *count=region->count;
  return RMA2_SUCCESS;
}

RMA2_Nodeid rma2_get_nodeid(RMA2_Port port)
{ 
  uint64_t value;
  int result=ioctl((port)->fd, RMA2_IOCTL_GET_NODEID, &value);
  if (result<0)
    {
      //perror("RMA ioctl GET_NODEID failed:");
      return RMA2_ERR_ERROR;      
  }
  (port)->nodeid=value;
  return port->nodeid;
}

RMA2_VPID rma2_get_vpid(RMA2_Port port)
{ 
  return port->vpid;
}

RMA2_Nodeid rma2_get_node_num(RMA2_Port port)
{ 
  return port->max_nodeid;
}

RMA2_VPID rma2_get_proc_num(RMA2_Port port)
{ 
  return port->max_vpid;
}

// void* rma2_get_tlb(RMA2_Port port)
// {return 0;
//   return port->STT;
// }

void rma2_serror (RMA2_ERROR error, char* buf, int buflen)
{
	switch (error) {
	  case RMA2_SUCCESS: strncpy(buf, "Success",buflen); break;
	case RMA2_ERR_ERROR : strncpy(buf, "An unknown error occured",buflen); break; //!< severe error or unkown error, should never occur
	case RMA2_ERR_INV_HANDLE : strncpy(buf, "Invalid virtual connection handle",buflen); break; //!< Invalid virtual connection handle
	case RMA2_ERR_INV_PORT: strncpy(buf, "Invalid port",buflen); break; //!< Invalid Port.
	case RMA2_ERR_INV_VALUE: strncpy(buf, "Invalid parameter value",buflen); break; //!< invalid parameter value
	case RMA2_ERR_NO_ROUTE: strncpy(buf, "No route to destination available",buflen); break; //!< No route can be found!
	case RMA2_ERR_PORTS_USED: strncpy(buf, "All ports already used",buflen); break; //!< All ports used.
	case RMA2_ERR_NO_MEM: strncpy(buf, "Memory could not be registered",buflen); break;//!< Memory can not be registered 
	case RMA2_NO_NOTI: strncpy(buf, "Currently no notification is available (maybe try later?)",buflen); break; //!< There was no notification.
	case RMA2_ERR_POLL_EXCEEDED: strncpy(buf, "Poll limit exceeded",buflen); break; //!< poll limit exceeded.
	case RMA2_ERR_WRONG_ARG: strncpy(buf, "Invalid argument",buflen); break; //!< Wrong argument in an rma function call
	case RMA2_ERR_NOT_YET: strncpy(buf, "Requested feature not yet implemented",buflen); break; //!< requested feature is not yet implemented.
	case RMA2_ERR_NO_DESC_MEM: strncpy(buf, "Currenlty no free descriptor space available",buflen); break; //!< no free descriptor space available. (Try again later)
	case RMA2_ERR_NO_MATCH: strncpy(buf, "There was no matching notification",buflen); break; //!< There was no match for the notification
	case RMA2_ERR_WRONG_CMD: strncpy(buf, "Invalid command",buflen); break; //!< Wrong command 
	case RMA2_ERR_NO_DEVICE: strncpy(buf, "No EXTOLL device available",buflen); break; //!< No extoll device found.
	case RMA2_ERR_NO_CONNECT: strncpy(buf, "No connection",buflen); break; //!< No connection between these ports exists.
	case RMA2_ERR_INVALID_VERSION: strncpy(buf, "Version mismatch between OS driver and API library",buflen); break; //!< Version of driver and API library do not match
	case RMA2_ERR_IOCTL: strncpy(buf, "IOCTL failed",buflen); break;          //!< Ioctl to RMA device driver failed
	case RMA2_ERR_MMAP: strncpy(buf, "Mapping of requester MMIO space failed",buflen); break;            //!< mmap of requester address space failed
	case RMA2_ERR_FD: strncpy(buf, "Communication with the OS driver failed",buflen); break;   //!< opening of the RMA device special file failed
	case RMA2_ERR_MAP: strncpy(buf, "Could not allocate region or table",buflen); break; //!< memory allocation or free for Region map or table failed
	case RMA2_ERR_DOUBLE_MAP: strncpy(buf, "TLB error",buflen); break; //!< try to allocate a 2nd level Region table although one is already present
	case RMA2_ERR_NO_TABLE: strncpy(buf, "Translation table not present",buflen); break; //!< try to access or free a Region table or Region which is not present
	case RMA2_ERR_NOT_FREE: strncpy(buf, "Tried to free a ressource which is still inuse",buflen); break; //!< try to free a ressource which is still in use
	case RMA2_ERR_PART: strncpy(buf, "Tried to use a region with a partial match, where only a full match is allowed",buflen); break; //!< tried to use a region with a partial match, where only a full match is allowed
	case RMA2_ERR_ATU: strncpy(buf, "An Error related to an ATU operation occured",buflen); break; //!< Error related to an ATU operation occured
	case RMA2_ERR_MLOCKLIMIT: strncpy(buf, "the rlimit MLOCK is reached. Try to unregister regions before continuation",buflen); break; //!< Error means, that the rlimit MLOCK is reached. Try to unregister regions before continuation.  
	}
}

void rma2_perror(RMA2_ERROR error, char* header)
{
	char buf[128];
	rma2_serror(error,buf,128);
	if (header)
	  fprintf(stderr, "%s: %s\n",header, buf);
	else
	  fprintf(stderr, "%s\n",buf);
	
}

uint32_t rma2_get_queue_size(RMA2_Port port)
{
  uint64_t value;
  int result=_rma2_get_size(port, &value);
  if (result<0) return 0;
  else return value;
}

RMA2_ERROR rma2_set_queue_size(RMA2_Port port, uint32_t bytes)
{
  uint64_t size=bytes;
  int err;
  err=_rma2_queue_munmap(port);
  if (err) return err;
  err=_rma2_set_size(port, size);
  if (err) return RMA2_ERR_ERROR;
  port->queue.size=size/sizeof(RMA2_Notification);
  err=_rma2_queue_mmap(port);
  if (err) return err;
  return RMA2_SUCCESS;
}

uint32_t rma2_get_queue_segment_size(RMA2_Port port)
{
   uint64_t value;
   int err = ioctl(port->fd, RMA2_IOCTL_GET_NOTIQ_SEGMENT_SIZE, &value);
   if (err)
   {
	   //perror("RMA2: IOCTL(GET_SEGMET_SIZE):");
	   return 0;
   }
   return value;
}

RMA2_ERROR rma2_set_queue_segment_size(RMA2_Port port,uint32_t size)
{
  uint64_t value=size;
  int result=_rma2_set_segment_size(port, value);
  if (result<0)
    return RMA2_ERR_ERROR;   
  return RMA2_SUCCESS;
}

