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
//#define velo_recv velo_recv_new


void noti_put_pong(void)
{
 uint64_t value;
 uint8_t class;
 RMA2_Notification* notip;
 int i;

 for (i=0;i<noti_warmup;i++)
   {
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
#ifdef PHYS_NOTI     
     rc=rma2_post_notification(port, phys_handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#else     
     rc=rma2_post_notification(port, handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#endif     
   }


 for (i=0;i<noti_max;i++)
   {
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

#ifdef PHYS_NOTI
     rc=rma2_post_notification(port, phys_handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
#else     
     rc=rma2_post_notification(port, handle, class, value, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);     
#endif     
   }
}

void imm_put_pong(void)
{
 uint64_t value=0xDEADBEEFL;
 RMA2_Notification* notip;
 int i,j;
 int err=0;
 volatile uint64_t* rb;

 rb=(uint64_t*)recv_buffer;

 printf("Dest address is %lx\n",dest_address);
 for (i=0;i<imm_warmup;i++)
   {
     rc=rma2_noti_get_block(port, &notip);
//      if (rc!=RMA2_SUCCESS)
//        {
//          fprintf(stderr,"Notification failed (%d).\n",rc);
//          shutdown();
//        }
     rma2_noti_free(port,notip);     
     
     rc=rma2_post_immediate_put(port, handle, 7, value, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);   
   }
 //*rb=0;
 for (j=1;j<=8;j++){
  for (i=0;i<imm_max;i++)
   {
     rc=rma2_noti_get_block(port, &notip);
//      if (rc!=RMA2_SUCCESS)
//        {
//          fprintf(stderr,"Notification failed (%d).\n",rc);
//          shutdown();
//        }
     rma2_noti_free(port,notip);
//      if ( ((uint64_t*)recv_buffer)[0]==0) {
//        err++;
//      }
     //((uint64_t*)recv_buffer)[0]=0;
     //*rb=0;
     rc=rma2_post_immediate_put(port, handle, j, value, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);      
   }
  }
 *rb=0;
 //((uint64_t*)recv_buffer)[0]=0;
 printf("Immediate Put: Data errors: %u\n",err);
       
 for (j=1;j<=8;j++){
  //printf("working on size %d\n",j);
  for (i=0;i<imm_max;i++)
   {
     while (*rb==0) {};
     *rb=0;
/*      while ( ((uint64_t*)recv_buffer)[0]==0) {}; */
/*      ((uint64_t*)recv_buffer)[0]=0; */

     rc=rma2_post_immediate_put(port, handle, j,value, dest_address, RMA2_NO_NOTIFICATION,RMA2_CMD_DEFAULT);
    }
  }
 printf("done...\n");
}

void bt_put_pong(void)
{
 int i;
  int rep;
 RMA2_Notification* notip;
  
  printf("put_bt_pong()\n");
  //warmup
  for (rep=0;rep<put_warmup;rep++)
    {
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      //rma2_noti_dump(notip);
      rma2_noti_free(port,notip);
      rc=rma2_post_put_bt(port, handle, send_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);    
    }

  for (i=1;i<1024;i++)
    {
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
          rc=rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);

        }
    }
  for (i=2048;i<=64*1024;i=i+1024)
    {
          for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
          rc=rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);

        }
    }
}

void qw_put_pong(void)
{
 int i;
 int rep;
 RMA2_Notification* notip;
 
  
  //warmup
  for (rep=0;rep<put_warmup;rep++)
    {      
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
        {
          fprintf(stderr,"Notification failed (%d).\n",rc);
          shutdown();
        }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);

      rc=rma2_post_put_qw(port, handle, send_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
    }
  for (i=8;i<=1024;i=i+8)
    {
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);

          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
        }
    }
   for (i=2048;i<=64*1024;i=i+1024)
    {
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);

          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);
        }
    }
    printf("qw_put_pong done...\n");
}

void put_pong()
{
  int i;
  int rep;
  RMA2_Notification* notip;
 
  
  //warmup
  for (rep=0;rep<put_warmup;rep++)
    {
      rc=rma2_noti_get_block(port, &notip);
      if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
      //printf("Got a noti...\n");
      rma2_noti_free(port,notip);
      rc=rma2_post_put_qw(port, handle, send_region, 0, 8, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);     
    }

  for (i=1;i<=7;i++)
    {
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);

          rc=rma2_post_put_bt(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
    }
  for (i=8;i<1024;i=i+8)
    {
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
    }
  for (i=1024;i<64*1024;i=i+1024)
    {
      for (rep=0;rep<put_max;rep++)
        {
          rc=rma2_noti_get_block(port, &notip);
          if (rc!=RMA2_SUCCESS)
            {
              fprintf(stderr,"Notification failed (%d).\n",rc);
              shutdown();
            }
          //printf("Got a noti...\n");
          rma2_noti_free(port,notip);
          rc=rma2_post_put_qw(port, handle, send_region, 0, i, dest_address, RMA2_COMPLETER_NOTIFICATION,RMA2_CMD_DEFAULT);         
        }
    }
    printf("combined put done...\n");
}

void get_pong(void)
{
  RMA2_Notification* notip;
  
  printf("Ok, entering get_pong()\n");
  //actually, do nothing, as we are the passive process...
  
  //at the end sender send one responder noti as sync point
  rc=rma2_noti_get_block(port, &notip);
  if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      shutdown();
            }
  //printf("Got a noti...\n");
  rma2_noti_free(port,notip);
}


void noop(void)
{
  //noop
}


void stop_pong(void)
{
  printf("Exiting..\n");
}

void biput_streaming_pong(void)
{
  int i;
  int rep; 
  RMA2_Notification* notip;
  
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


  for (i=1;i<64;i++)
    {
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
    }
   for (i=64;i<1024;i=i+8)
    {
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
    }
   for (i=1024;i<64*1024;i=i+1024)
    {
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
    }
}

void biget_streaming_pong(void)
{
  int i;
  int rep; 
  RMA2_Notification* notip;
  
  //warmup
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
  rma2_noti_free(port,notip);


  for (i=1;i<64;i++)
    {
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
    }
  for (i=64;i<1024;i=i+8)
    {
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
    }
    for (i=1024;i<64*1024;i=i+1024)
    {
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
    }

}


void do_the_pong(void)
{
  //commands_t cmd;
  int cmd;
  
  while (1)
    {
      recv_start(&cmd);
      switch (cmd) {
      case NOTI_PUT:{
        noti_put_pong();
        break;
      } 
      case IMM_PUT:{
        imm_put_pong();
        break;
      }
      case BT_PUT: {
        bt_put_pong();
        break;
      } 
      case QW_PUT: {
        qw_put_pong();
        break;
      } 
      case PUT: {
        put_pong();
        break;
      } 
      case GET:{
        get_pong();
        break;
      }  
      case TP:
        {
          noop();
          break;
        }
      case STREAMINGPUT:
        {
          noop();
          break;
        }
      case STREAMINGGET:
        {
          noop();
          break;
        }
      case BISTREAMINGPUT:
        {
          biput_streaming_pong();
          break;
        }
      case STREAMINGIMMPUT:
        {
          noop();
          break;
        }
      case BISTREAMINGGET:
        {
          biget_streaming_pong();
          break;
        }
      case LOCK:
        {
          noop();
          break;
        }
      case STOP:
        {
          stop_pong();
          return;
        }
      default:
        {
          fprintf(stderr, "Received unknown command %u ignoring....\n",cmd);
        }
      }
    }
}
