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
#ifndef SYS_RMA2_H
#define SYS_RMA2_H

#ifdef __cplusplus
extern "C" {
#endif
#include <arch_define.h>

#include "extoll2_list.h"


/*!
  \defgroup INTERNAL      Internal data structures, definitions etc. for RMA

  This group contains data structures , that are internally used by the RMA API, but not exported.

  @{
*/

#define RMA2_RP_LAZYNESS 32

#ifdef MICEXTOLL
        #define flush_wc() 
#else
 #ifdef LS3BEXTOLL
        #define flush_wc() 
 #else
        #define flush_wc() asm volatile("sfence" ::: "memory")
 #endif
#endif

//#define mov128(dest, source) asm volatile("movq %0,%%rax \n movdqa %1,%%xmm0 \n movntdq %%xmm0,(%%rax)" :: "m"(dest), "m"(source))

#define INKRP(rp,size) rp=(rp+1) % size
#define INKWP(wp,size) wp=(wp+1) % size
#define UPDATE_RP(rp_bak, rp , rp_wb,size) if ( (rp_bak - rp)%size >RMA2_RP_LAZYNESS) {  DEBUG(printf("updated rp from %x to hex %lx\n", rp_bak, (((uint64_t)rp)));)  *rp_wb=(uint64_t) (((uint64_t)rp)); rp_bak=rp; }


typedef struct {
  RMA2_Notification* queue;
  uint32_t size;
  uint32_t rp;
  uint32_t rp_bak;
  volatile uint64_t* rp_wb;
  //uint32_t* rp_wb;
} RMA2_noti_queue;

typedef struct {
  struct extoll_list_head entry;
  RMA2_Notification noti;
} RMA2_noti_item;

//struct _RMA2_RME;

typedef struct {
  RMA2_noti_queue queue;
  //struct list_head used_list;
  //struct list_head free_list;
  extoll2_list_t* descriptor_queue;
  extoll2_list_t* region_queue;

  //void* STT;
  int fd;
  uint32_t pad0;

  void* desc_pool;
  //int* desc_free_list;
  //int desc_free_count;

  uint32_t nodeid;
  uint32_t vpid;
  uint32_t max_nodeid;
  uint32_t max_vpid;

  //struct _RMA2_RME* region_map;
  RMA2_Region* regions;
  RMA2_Region* freed_regions;

  size_t registered_mem;
  size_t limit;
  
  volatile void* req_pages[32]; //all the requester pages for all the options
  
  //flow control related fields
  volatile uint64_t *state_map;	//! mapped memory of wcbuf state
  uint64_t* packet_buffer; //! buffer to save packets to be sent
  uint32_t committed_count;
  uint32_t packet_wp;
  uint64_t retried_packet_count;
  RMA2_Replay_Buffer_Mode replay_mode;


} RMA2_Endpoint;

typedef struct {
  uint32_t nodeid;
  uint32_t vpid;
  volatile uint64_t* req_page;
  
} RMA2_Connection;

#ifdef  __cplusplus
}
#endif

#endif
