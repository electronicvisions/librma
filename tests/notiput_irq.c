#include <stdio.h>
#include <rma2.h>

int main()
{
  RMA2_Port port_send;
  RMA2_Port port_recv;
  RMA2_Handle handle;
  RMA2_VPID dest_vpid;
  RMA2_Nodeid dest_nodeid;
  uint64_t value = 42;
  //RMA2_Notification* notification;
 
  rma2_open(&port_send);
  rma2_open(&port_recv);

  dest_vpid = rma2_get_vpid(port_recv);
  dest_nodeid = rma2_get_nodeid(port_recv);

  printf("sending vpid: %i, receiving vpid %i\n", rma2_get_vpid(port_send), dest_vpid);

  rma2_connect(port_send, dest_nodeid, dest_vpid, RMA2_CONN_IRQ, &handle);

  rma2_post_notification(port_send, handle, 43, value, RMA2_ALL_NOTIFICATIONS, 0);

  printf("Please check kernel log for SNQ Errors.\n");

  rma2_disconnect(port_send, handle);
  rma2_close(port_recv);
  rma2_close(port_send);

  return 0;
}
