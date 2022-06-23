/*
 * Example program for the LIBRMA2 API.
 * Opens RMA and sends itsself a message.
 *
 * Author: Tobias Groschup
 */


#include <rma2.h>

#include <stdlib.h>
#include <stdio.h>

#define LARGE_NUMBER 128
//128 is now a large number

int main(int argc, char ** argv)
{
  RMA2_Port port;
  RMA2_Handle handle;
  RMA2_Notification *notip;
  RMA2_Region *send_region;
  RMA2_Region *recv_region;
  RMA2_NLA dest_address;
  RMA2_ERROR rc;

  RMA2_Nodeid destination_node;
  RMA2_VPID destination_vpid;

  int offset = 0;
  int size = LARGE_NUMBER;
  char *send_buffer[LARGE_NUMBER];
  char *recv_buffer[LARGE_NUMBER];

  memset(send_buffer, 1, LARGE_NUMBER);
  memset(recv_buffer, 0, LARGE_NUMBER);

  //open the RMA device
  rc = rma2_open(&port);
  if(rc != RMA2_SUCCESS) {
    fprintf(stderr,"RMA open failed (%d)", rc);
    abort();
  }

  //get the local node id and the process' VPID
  destination_node = rma2_get_nodeid(port);
  destination_vpid = rma2_get_vpid(port);

  //connect to self
  rc = rma2_connect(port, destination_node, destination_vpid, RMA2_CONN_DEFAULT, &handle);
  if(rc != RMA2_SUCCESS) {
    fprintf(stderr,"RMA connect failed (%d)", rc);
    rma2_close(port);
    abort();
  }
  
  //regsiter the regions
  rc = rma2_register(port, send_buffer, size, &send_region);
  if (rc != RMA2_SUCCESS) {
    fprintf(stderr,"Send Buffer registration failed (%d).\n",rc);
    rma2_close(port);
    abort();
  }
  rc = rma2_register(port, recv_buffer, size, &recv_region);
  if (rc != RMA2_SUCCESS) {
    fprintf(stderr,"Receive Buffer registration failed (%d).\n",rc);
    rma2_close(port);
    abort();
  }

  //get NLA of receive region 
  rc = rma2_get_nla(recv_region, 0, &dest_address);
  if(rc != RMA2_SUCCESS) {
    fprintf(stderr,"Error while reading destination NLA.\n");
    rma2_close(port);
    abort();
  }

  //put some data to the recv_region
  rc = rma2_post_put_bt(port, handle, send_region, offset, size, dest_address, RMA2_COMPLETER_NOTIFICATION | RMA2_REQUESTER_NOTIFICATION, RMA2_CMD_DEFAULT);
  if (rc != RMA2_SUCCESS) {
    fprintf(stderr,"Posting of put operation failed (%d).\n",rc);
    rma2_close(port);
    abort();
  }

  //wait for operation to be complete
  rc = rma2_noti_get_block(port, &notip);
  if (rc != RMA2_SUCCESS) {
    fprintf(stderr,"Notification failed (%d).\n",rc);
    rma2_close(port);
    abort();
  }
  rma2_noti_free(port, notip);

  
  //check, if message was received correctly
  if(memcmp(send_buffer, recv_buffer, LARGE_NUMBER) == 0) {
    printf("RMA example was successful!\n");
  }else{
    printf("Error in RMA Example: Received values does not match sent value .\n");
  }
  
  // unregister regions
  rma2_unregister(port, send_region);
  rma2_unregister(port, recv_region);

  //close connection
  rma2_disconnect(port, handle);

  //close device
  rma2_close(port);

  return 0;
}

