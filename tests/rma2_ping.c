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

//now follow the different protocols...


void noti_put_ping(void)
{
 uint64_t value=0xDEADBEEFL;
 uint8_t class=0x12;
 RMA2_Notification* notip;
 int i;

 send_start(NOTI_PUT,noti_max);
 wait_start_ack();
 for (i=0;i<noti_warmup;i++)
   {
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
       }
     pp_dest_node=rma2_noti_get_remote_nodeid(notip);
     dest_vpid=rma2_noti_get_remote_vpid(notip);
     value=rma2_noti_get_notiput_payload(notip);//->operand.immediate_value;
     class=rma2_noti_get_notiput_class(notip);
     rma2_noti_free(port,notip);     
   }

 PUTCMD("NOTIFICATION PUT","notiput");
 STARTTIMER;
 for (i=0;i<noti_max;i++)
   {
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
       }
     pp_dest_node=rma2_noti_get_remote_nodeid(notip);
     dest_vpid=rma2_noti_get_remote_vpid(notip);
     value=rma2_noti_get_notiput_payload(notip);//->operand.immediate_value;
     class=rma2_noti_get_notiput_class(notip);
     rma2_noti_free(port,notip);
     
   }
 STOPTIMER;
 PUTTIME(noti_max, 9);
}


void imm_put_ping(void)
{
 uint64_t value=0xDEADBEEFL;
 RMA2_Notification* notip;
 int i,j;
 volatile uint64_t* rb;

 rb=(uint64_t*)recv_buffer;

 send_start(IMM_PUT,imm_max);
 wait_start_ack();

 printf("Dest address is %lx, value is %lx\n",dest_address,value);
 for (i=0;i<imm_warmup;i++)
   {
     rma2_post_immediate_put(port, handle, 7, value, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
     rc=rma2_noti_get_block(port, &notip);
     //rma2_noti_dump(notip);     
//      if (rc!=RMA2_SUCCESS)
//        {
//          fprintf(stderr,"Notification failed (%d).\n",rc);
//          shutdown();
//        }
     rma2_noti_free(port,notip);     
   }
 //((uint64_t*)recv_buffer)[0]=0;
 printf("Dest address is %lx, value is %lx\n",dest_address,value);
 PUTCMD("IMMEDIATE PUT (with completer notification)","immput");
 printf("Dest address is %lx, value is %lx\n",dest_address,value); 
 
 for (j=1;j<=8;j++)
 {
 STARTTIMER;
 for (i=1;i<=imm_max;i++)
   {
     rma2_post_immediate_put(port, handle,  j, value, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);

     rc=rma2_noti_get_block(port, &notip);
     //rma2_noti_dump(notip);     
//      if (rc!=RMA2_SUCCESS)
//        {
//          fprintf(stderr,"Notification failed (%d).\n",rc);
//          shutdown();
//        }
     //pp_dest_node=notip->source_nodeid;
     //dest_vpid=notip->source_vpid;
     rma2_noti_free(port,notip);
//      if ( ((uint64_t*)recv_buffer)[0]==0) {
//        printf("Immediate Put: Data error: %lx\n", ((uint64_t*)recv_buffer)[0]);
     //}
     //((uint64_t*)recv_buffer)[0]=0;     
   }
 STOPTIMER;
 //((uint64_t*)recv_buffer)[0]=0;
 *rb=0;
 PUTTIME(imm_max, j);
  }
 PUTCMD("IMMEDIATE PUT (without completer notification)","immput_nonoti");
 
 for (j=1;j<=8;j++)
 {
   //printf("working on size %d\n",j);
 STARTTIMER;
 for (i=0;i<imm_max;i++)
   {
     rma2_post_immediate_put(port, handle, j, value, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);

     while (*rb==0) {};
     *rb=0;
/*      while ( ((uint64_t*)recv_buffer)[0]==0) {}; */
/*      ((uint64_t*)recv_buffer)[0]=0; */
     
   }
 STOPTIMER;
 PUTTIME(imm_max, j);
 }
}

void bt_put_ping(void)
{
  int i;
  int rep;
  RMA2_Notification* notip;
 
  //warmup
  send_start(BT_PUT,put_max);
  wait_start_ack();
  for (rep=0;rep<put_warmup;rep++)
    {
      rc=rma2_post_put_bt(port, handle, send_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      if (rc!=RMA2_SUCCESS)
      {
        fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
        shutdown();
      }      
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }

      //printf("Got a noti...\n");
      //rma2_noti_dump(notip);
      rma2_noti_free(port,notip);
    }

  PUTCMD("RMA Byte PUT", "btput");
  for (i=1;i<1024;i++)
    {
      STARTTIMER;
      for (rep=0;rep<put_max;rep++)
        {
          rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
	  {
	    fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
	    shutdown();
	  }      
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }

          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      PUTTIME(put_max, (i));
    }
   for (i=2048;i<=64*1024;i=i+1024)
    {
      STARTTIMER;
      for (rep=0;rep<put_max;rep++)
        {
          rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
	  {
	    fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
	    shutdown();
	  }      
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }

          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      PUTTIME(put_max, (i));
    }
}

void qw_put_ping(void)
{
 int i;
 int rep;
 RMA2_Notification* notip;
  
  //warmup
  send_start(QW_PUT,put_max);
  wait_start_ack();
  for (rep=0;rep<put_warmup;rep++)
    {
      rc=rma2_post_put_qw(port, handle, send_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      if (rc!=RMA2_SUCCESS)
      {
        fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
        shutdown();
      }      
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }

      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
    }
  PUTCMD("RMA Cacheline PUT","qwput");
  for (i=8;i<=1024;i=i+8)
    {
      STARTTIMER;
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
	  {
	    fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
	    shutdown();
	  }      	  
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      PUTTIME(put_max, (i));
    }
  for (i=2048;i<=64*1024;i=i+1024)
    {
      STARTTIMER;
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
	  {
	    fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
	    shutdown();
	  }      	  
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      PUTTIME(put_max, (i));
    }
}

void put_ping(void)
{
  int i;
  int rep; 
  RMA2_Notification* notip;
  
  //warmup
  send_start(PUT,put_max);
  wait_start_ack();
  for (rep=0;rep<put_warmup;rep++)
    {
      rc=rma2_post_put_qw(port, handle, send_region, 0, 128*1024, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
              shutdown();
            }      
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
    }

  PUTCMD("RMA PUT","put");
  for (i=1;i<=7;i++)
    {
      STARTTIMER;
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
              shutdown();
            }      	  
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      PUTTIME(put_max, (i) );
    }
  for (i=8;i<1024;i=i+8)
    {
      STARTTIMER;
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
              shutdown();
            }      
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      PUTTIME(put_max, (i));
    }
  for (i=1024;i<1024*64;i=i+1024)
    {
      STARTTIMER;
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Posting of PUT failed (%d).\n",rc);
              shutdown();
            }      
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      PUTTIME(put_max, (i));
    }    

}

void get_ping(void)
{
  int i;
  int rep;
  RMA2_Notification* notip;
  
  //warmup
  send_start(GET,get_max);
  wait_start_ack();
  for (rep=0;rep<get_warmup;rep++)
    {
      //printf("posted get..\n");
      rc=rma2_post_get_qw(port, handle, recv_region, 0, 128*1024, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Posting of GET failed (%d).\n",rc);
          shutdown();
        }
      //printf("posted\n");
      rc=rma2_noti_get_block(port, &notip);
      //printf("got noti\n");
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
    }
  printf("get warmup done\n");
   PUTCMD("RMA GET","get");
  for (i=1;i<=7;i++)
    {
      STARTTIMER;
      for (rep=0;rep<get_max;rep++)
        {
          rc=rma2_post_get_bt(port, handle, recv_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
	  {
	    fprintf(stderr,"Posting of GET failed (%d).\n",rc);
	    shutdown();
	  }
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      GETTIME(get_max, (i) );
    }
//   printf("get bt done\n");
  for (i=8;i<1024;i=i+8)
    {
      STARTTIMER;
      for (rep=0;rep<get_max;rep++)
        {
          rc=rma2_post_get_qw(port, handle, recv_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
	  {
	    fprintf(stderr,"Posting of GET failed (%d).\n",rc);
	    shutdown();
	  }
	  //printf("wait for noti\n");
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      GETTIME(get_max, (i));
    }
    for (i=1024;i<=64*1024;i=i+1024)
    {
      STARTTIMER;
      for (rep=0;rep<get_max;rep++)
        {
          rc=rma2_post_get_qw(port, handle, recv_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
	  if (rc!=RMA2_SUCCESS)
	  {
	    fprintf(stderr,"Posting of GET failed (%d).\n",rc);
	    shutdown();
	  }
	  //printf("wait for noti\n");
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
        }
      STOPTIMER;
      GETTIME(get_max, (i));
    }


  //signal the end of the getting using a responder notification
  rc=rma2_post_get_qw(port, handle, recv_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION | RMA2_RESPONDER_NOTIFICATION,RMA2_CMD_DEFAULT);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"Posting of GET failed (%d).\n",rc);
    shutdown();
  }
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
  {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
  }
  //printf("Got a noti...\n");
  rma2_noti_free(port,notip);
}


void put_throughput_ping(void)
{
 uint64_t value=0xDEADBEEFL;
 int i;
 RMA2_Notification* notip;
 // RMA2_Notification* notip;

 send_start(TP,tp_max);
 wait_start_ack();
 for (i=0;i<tp_warmup-1;i++)
   {
     rma2_post_immediate_put(port, handle, 8, value, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);    
     //printf("send...\n");
   }
 rma2_post_immediate_put(port, handle,  8, value, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);    
  /*  printf("done...\n"); */
  rc=rma2_noti_get_block(port, &notip); 
   if (rc!=RMA2_SUCCESS) 
     { 
       fprintf(stderr,"Notification failed (%d).\n",rc); 
       shutdown(); 
     }  
  rma2_noti_free(port,notip);
 PUTCMD("Operation Throughput (immediate put without completer notification), MOP/s given","immput_nonoti_throughput");
 STARTTIMER;
 // printf("ops: %u\n",tp_max);
 for (i=0;i<tp_max-1;i++)
   {
     rma2_post_immediate_put(port, handle, 8, value, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);    
     //printf("send...\n");
   }
 rma2_post_immediate_put(port, handle,  8, value, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);    
  /*  printf("done...\n"); */
  rc=rma2_noti_get_block(port, &notip); 
   if (rc!=RMA2_SUCCESS) 
     { 
       fprintf(stderr,"Notification failed (%d).\n",rc); 
       shutdown(); 
     }  
  rma2_noti_free(port,notip);
  STOPTIMER; 
  /*  printf("jupp\n"); */
 GETTIME(tp_max, 1); //one because this are mops per second
 //printf("nono\n");
}

void put_streaming_imm_put(void)
{  
  int i;
  int rep; 
  RMA2_Notification* notip;
  uint64_t value=0xDEADBEEFl;
  
  //warmup
  send_start(STREAMINGIMMPUT,put_max);
  wait_start_ack();
  for (rep=0;rep<put_warmup;rep++)
    {     
	   rma2_post_immediate_put(port, handle, 7, value, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
    }
  rma2_post_immediate_put(port, handle,  7, value, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);      
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
    }
  //printf("Got a noti...\n");
  rma2_noti_free(port,notip);


  PUTCMD("RMA streaming Immediate PUT bandwidth","streamingimmput");
  for (i=1;i<8;i++)
    {
      STARTTIMER;
      for (rep=0;rep<imm_put_max;rep++)
        {          
		  rma2_post_immediate_put(port, handle,  i, value, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
        }	  
      STOPTIMER;
      GETTIME(imm_put_max, i);
// 	  for (rep=0;rep<1000;rep++)
// 	  {
//         rma2_post_immediate_put(port, handle,  i, &value, dest_address, RMA2_REQUESTER_NOTIFICATION);     
//         usleep(10);
//         STARTTIMER;
//         rc=rma2_noti_get_block(port, &notip);
//         if (rc!=RMA2_SUCCESS)
//         {
//           fprintf(stderr,"Notification failed (%d).\n",rc);
//           shutdown();
//         }      	  	  
//       rma2_noti_free(port,notip);
//       STOPTIMER;
// 	  GETTIME(1, 0);
//	  }
    }
}

void put_streaming_ping(void)
{
  int i;
  int rep; 
  RMA2_Notification* notip;
  
  //warmup
  send_start(STREAMINGPUT,put_max);
  wait_start_ack();
  for (rep=0;rep<put_warmup;rep++)
    {
      rma2_post_put_qw(port, handle, send_region, 0, 8, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
    }
  rma2_post_put_qw(port, handle, send_region, 0, 8, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
    }  
  rma2_noti_free(port,notip);


  PUTCMD("RMA streaming PUT bandwidth","streamingput");
  for (i=1;i<64;i++)
    {
      STARTTIMER;
      for (rep=0;rep<put_max-1;rep++)
        {
          rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);        
        }
      rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);        
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(put_max, (i) );
    }
  for (i=64;i<1024;i=i+8)
    {
      STARTTIMER;
      for (rep=0;rep<put_max-1;rep++)
        {
          rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
      rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);         
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(put_max, (i));
    }
  for (i=1024;i<64*1024;i=i+1024)
    {
      STARTTIMER;
      for (rep=0;rep<put_max-1;rep++)
        {
          rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
      rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);         
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(put_max, (i) );
    }
}

void get_streaming_ping(void)
{
  int i;
  int rep; 
  RMA2_Notification* notip;
  
  //warmup
  send_start(STREAMINGGET,get_max);
  wait_start_ack();
  for (rep=0;rep<get_warmup;rep++)
    {
      rma2_post_get_qw(port, handle, send_region, 0, 8, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
    }
  rma2_post_get_qw(port, handle, send_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
    }
  //printf("Got a noti...\n");
  rma2_noti_free(port,notip);


  PUTCMD("RMA Streaming GET Bandwidth","streamingget");
  for (i=1;i<64;i++)
    {
      STARTTIMER;
      for (rep=0;rep<get_max-1;rep++)
        {
          rma2_post_get_bt(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);        
        }
      rma2_post_get_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);        
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(get_max, (i) );
    }
  for (i=64;i<1024;i=i+8)
    {
      STARTTIMER;
      for (rep=0;rep<get_max-1;rep++)
        {
          rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
      rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);         
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(get_max, (i));
    }
   for (i=1024;i<64*1024;i=i+1024)
    {
      STARTTIMER;
      for (rep=0;rep<get_max-1;rep++)
        {
          rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
      rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);         
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(get_max, (i) );
    }

}


void biput_streaming_ping(void)
{
  int i;
  int rep; 
  RMA2_Notification* notip;
  
  //warmup
  send_start(BISTREAMINGPUT,put_max);
  wait_start_ack();
  for (rep=0;rep<put_warmup;rep++)
    {
      rma2_post_put_qw(port, handle, send_region, 0, 8, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
    }
  rma2_post_put_qw(port, handle, send_region, 0, 8, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
    }
  //printf("Got a noti...\n");
  rma2_noti_free(port,notip);


  PUTCMD("RMA streaming PUT bandwidth","bistreamingput");
  for (i=1;i<64;i++)
    {
      STARTTIMER;
      for (rep=0;rep<put_max-1;rep++)
        {
          rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);        
        }
      rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);        
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(put_max, (i));
    }
  for (i=64;i<1024;i=i+8)
    {
      STARTTIMER;
      for (rep=0;rep<put_max-1;rep++)
        {
          rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
      rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);         
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(put_max, (i));
    }
   for (i=1024;i<64*1024;i=i+1024)
    {
      STARTTIMER;
      for (rep=0;rep<put_max-1;rep++)
        {
          rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
      rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_REQUESTER_NOTIFICATION,RMA2_CMD_DEFAULT);         
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(put_max, (i));
    }
}

void biget_streaming_ping(void)
{
  int i;
  int rep; 
  RMA2_Notification* notip;
  
  //warmup
  send_start(BISTREAMINGGET,get_max);
  wait_start_ack();
  for (rep=0;rep<get_warmup;rep++)
    {
      rma2_post_get_qw(port, handle, send_region, 0, 8, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
    }
  rma2_post_get_qw(port, handle, send_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
    }
  //printf("Got a noti...\n");
  rma2_noti_free(port,notip);


  PUTCMD("RMA Streaming GET Bandwidth","bistreamingget");
  for (i=1;i<64;i++)
    {
      STARTTIMER;
      for (rep=0;rep<get_max-1;rep++)
        {
          rma2_post_get_bt(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
        }
      rma2_post_get_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(get_max, (i) );
    }
  for (i=64;i<1024;i=i+8)
    {
      STARTTIMER;
      for (rep=0;rep<get_max-1;rep++)
        {
          rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
        }
      rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(get_max, (i));
    }
    for (i=1024;i<64*1024;i=i+1024)
   {
      STARTTIMER;
      for (rep=0;rep<get_max-1;rep++)
        {
          rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
        }
      rma2_post_get_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      STOPTIMER;
      GETTIME(get_max, (i));
    }

}


void lock_ping(void)
{
  //int i;
  int rep;
  RMA2_Notification* notip;
  
  //warmup
  send_start(LOCK,lock_max);
  wait_start_ack();
  for (rep=0;rep<get_warmup;rep++)
    {
      //printf("posted get..\n");      
	  rma2_post_lock(port,handle, 0,0, 1000000, 1, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
    }
  PUTCMD("RMA lock","lock");
  STARTTIMER;
  for (rep=0;rep<get_max;rep++)
  {
    rma2_post_lock(port,handle, 0,0, 1000000, 1, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);    
    rc=rma2_noti_get_block(port, &notip);
    if (rc!=RMA2_SUCCESS)
      {
        fprintf(stderr,"Notification failed (%d).\n",rc);
        shutdown();
      }
    //printf("Got a noti...\n");
    rma2_noti_free(port,notip);
   }
   STOPTIMER;
   GETTIME(get_max, 4);
   
}

void ping_stop()
{
  printf("Request shutdown...\n");
  send_start(STOP,0);
  wait_start_ack();
  printf("Exiting...\n");
}

void print_list(void)
{
  printf("l\tPrint list of benchmarks.\n");
  printf("q\tQuit.\n\n");
  printf("1\tRMA Notification put ping-pong\n");
  printf("2\tRMA Immediate put pin-pong\n");
  printf("3\tRMA Byte put pin-pong\n");
  printf("4\tRMA Quadword put pin-pong\n");
  printf("5\tRMA combined put pin-pong\n");
  printf("6\tRMA combined get pin-pong\n");
  printf("b\tMax. put througput\n");
  printf("c\tPut streaming bandwidth\n");
  printf("d\tGet streaming bandwidth\n");
  printf("f\tBidirectional Put streaming bandwidth\n");
  printf("g\tBidirectional Get streaming bandwidth\n");
  printf("j\tStreaming Immediate Put\n");
  printf("r\tRemote Lock (atomic) operation\n");
}

void do_the_ping(void)
{
  char selection;
  while(1)
    {
      printf("Which benchmark do you want to run? (Please enter the number, enter l for a list of benchmarks and their associated numbers, q for quit)\n");
      do
      selection=getchar();
      while (isspace(selection));
      while (getchar() != '\n');
      switch (selection)
      {   
        case 'l': print_list(); break;
        case '1': noti_put_ping(); break;
        case '2': imm_put_ping(); break;
        case '3': bt_put_ping(); break;
        case '4': qw_put_ping(); break;
        case '5': put_ping(); break;
        case '6': get_ping(); break;
        case 'b': put_throughput_ping(); break;
        case 'c': put_streaming_ping(); break;
        case 'd': get_streaming_ping(); break;
        case 'f': biput_streaming_ping(); break;
        case 'g': biget_streaming_ping(); break;
        case 'j': put_streaming_imm_put();break;		
	case 'r': lock_ping(); break;


        case 'q':ping_stop(); return; break;
      }
    }
}
 
