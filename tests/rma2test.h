/* ============================================================
*
* Copyright (c) 2008, 2010 Computer Architecture Group, University of Heidelberg
*
* All rights reserved.
*
* University of Heidelberg
* Computer Architecture Group
* B6 26
* 68131 Mannheim
* Germany
* http://ra.ziti.uni-heidelberg.de/
*
* Author     :  Mondrian Nuessle
* Create Date:  27/10/08
* Design Name:  
* Module Name:  EXTOLL PingPong Test
* Description:  
*
* Revision:     Revision 1.0
*
* ===========================================================*/
#ifndef PING_PONG_H
#define PING_PONG_H
#include <arch_define.h>

#include <stdio.h>
#include <string.h>
#include <rma2.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <fcntl.h>


#define MB8 (8*1024*1024)

#define tiny_max    100
#define tiny_warmup 100
#define small_max    100
#define small_warmup 100
#define medium_max    100
#define medium_warmup 4
#define large_max    100
#define large_warmup 4

#define noti_max        10000
#define noti_warmup     100
#define imm_max         10000
#define imm_warmup      10
#define imm_put_max     10000
#define put_warmup      10
#define put_max         10000
#define get_warmup      10
#define get_max         1000
#define lock_max        10
#define tp_warmup       100
#define tp_max          1000

#define DEBUG(a)
//#define DEBUG(a) a

#ifdef MICEXTOLL
  #define rdtscll(val) do { \
       unsigned int __a,__d; \
       asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
       (val) = ((unsigned long long)__a) | (((unsigned long long)__d)<<32); \
} while(0)      
#else
 #ifdef LS3BEXTOLL
   extern volatile unsigned char *ls3_rdtsc_vaddr;
    
   #define rdtscll(val) (val)=(*(unsigned long *)ls3_rdtsc_vaddr);
 #else
  #define rdtscll(val) do { \
       unsigned int __a,__d; \
       asm volatile("rdtsc" : "=a" (__a), "=d" (__d)); \
       (val) = ((unsigned long long)__a) | (((unsigned long long)__d)<<32); \
} while(0)
 #endif
#endif
//#define PHYS_NOTI

//command encoding for internal protocol
enum _commands {
  NOTI_PUT=2, 
  IMM_PUT=3,
  BT_PUT=4, 
  QW_PUT=5, 
  PUT=6, 
  GET=7, 
  TINY=8, 
  SMALL=9, 
  MEDIUM=10,
  LONG=11,
  TP=12,
  STREAMINGPUT=13,
  STREAMINGGET=14,
  BISTREAMINGPUT=16,
  BISTREAMINGGET=17,
  LOCK=20,
  STREAMINGIMMPUT=22,

  STOP=23
};
typedef enum _commands commmands_t;

extern uint64_t time_start,time_stop;
#define STARTTIMER rdtscll(time_start)
#define STOPTIMER rdtscll(time_stop)

extern double usec;
extern double bw;

extern FILE* logfd;
extern char logbase[32];
extern char logname[255];

#define PUTTIME(repetitions, payload_size) \
  usec = (1.0*(time_stop-time_start)) / cycles_per_usec / repetitions / 2.0;  \
  bw= (1.0*payload_size)/usec; \
  printf("%u: %u %2lf %2lf\n", payload_size, repetitions, usec, bw); \
  fprintf(logfd,"%u:\t%u\t%2lf\t%2lf\n", payload_size, repetitions, usec, bw);

#define GETTIME(repetitions, payload_size) \
  usec = (1.0*(time_stop-time_start)) / cycles_per_usec / repetitions ;  \
  bw= (1.0*payload_size)/usec; \
  printf("%u: %u %2lf %2lf\n", payload_size, repetitions, usec, bw); \
  fprintf(logfd, "%u:\t%u\t%2lf\t%2lf\n", payload_size, repetitions, usec, bw);

#define PUTCMD(PROT, LOG) printf("\n\nPing-Pong Test for %s\n----------------------------------------------\n\n", PROT); \
  if (logfd!=0) fclose(logfd); \
  strcpy(logname, LOG); \
  strcat(logname, logbase); \
  printf ("opening logfile %s\n",logname); \
  logfd=fopen(logname, "w"); \
  if (logfd==0) { perror("Opening logfile"); shutdown(); abort(); }


extern unsigned char* send_buffer;
extern unsigned char* recv_buffer;

extern RMA2_Nodeid pp_dest_node;
extern RMA2_VPID dest_vpid;
extern RMA2_Nodeid own_node;
extern RMA2_VPID own_vpid;

extern RMA2_Region* send_region;
extern RMA2_Region* recv_region;
extern RMA2_NLA dest_address;

extern RMA2_Port port;
extern RMA2_Handle handle;
#ifdef PHYS_NOTI
extern RMA2_Handle phys_handle;
#endif

extern RMA2_ERROR rc;
extern double cycles_per_usec;

/* Test functions */
void do_the_ping(void);
void do_the_pong(void);

void shutdown(void);
void ping(void);
void pong(void);

void send_start(int cmd,  int repetitions);
void recv_start(int* cmd);
void wait_start_ack();

#endif
