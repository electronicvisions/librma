#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <rma2.h>

int main(int argc, char** argv)
{
  RMA2_Port port;
  RMA2_Handle handle;
  RMA2_ERROR error;
  RMA2_Notification *noti;

  int decrement = 0;

  error = rma2_open(&port);
  if(error) {
    printf("open: error nr: %i\n", error);
    abort();
  }

  error = rma2_connect(port, rma2_get_nodeid(port), rma2_get_vpid(port), RMA2_CONN_DEFAULT, &handle);
  if(error) {
    printf("connect error; %i\n", error);
    rma2_close(port);
    abort();
  }
  
  int lock_number = 7;
  int lock_value = 28;
  int lock_increment = 7;

  //test the locking: add to the lock value untill the lock locks
  for(int j=0; j<5; j++) {
    if(decrement) {
    error = rma2_post_lock(port, handle, 0, lock_number, lock_value, (-1)*lock_increment, RMA2_ALL_NOTIFICATIONS, RMA2_CMD_DEFAULT);
    } else {
    error = rma2_post_lock(port, handle, 0, lock_number, lock_value, lock_increment, RMA2_ALL_NOTIFICATIONS, RMA2_CMD_DEFAULT);
    }
    assert(error == RMA2_SUCCESS);
    
    //check if all returned notifications are correct
    for(int i=0;i<3;i++) {
      int num = 0, result =-1, value = -1, mult = 0; 

      error = rma2_noti_get_block(port, &noti);
      assert(error == RMA2_SUCCESS);

      //rma2_noti_dump(noti);

      num = rma2_noti_get_lock_number(noti);
      result = rma2_noti_get_lock_result(noti);
      value = rma2_noti_get_lock_value(noti);
      assert(num == lock_number);

      if(j*lock_increment <= lock_value) { 
        //success
        if(i==0) { //requester notification
          assert(result == 0);
          assert(value == lock_value);
        } else { // other notifications
          assert(result == 1);
          if(j<(lock_value/lock_increment+1)) {
            mult = j+1;
          } else {
            mult = (lock_value/lock_increment+1);
          }
          assert(value == mult*lock_increment);
        }
      } else {
        //failure
        assert(result == 0);
        if(i==0) { //again, first the responder notification
          assert(value == lock_value);
        } else { //other notifications
          assert(value == (lock_value/lock_increment+1)*lock_increment);
        }
      }

      error = rma2_noti_free(port, noti);
      assert(error == RMA2_SUCCESS);
    }
  }

  //decrement lock and check returned notifications 
  error = rma2_post_lock(port, handle, 0, lock_number, lock_value+lock_increment, -1*lock_increment, RMA2_ALL_NOTIFICATIONS, RMA2_CMD_DEFAULT);
  for(int i=0;i<3;i++) {
      int result=-1, value=-1, num=-1, tmp=0;
      error = rma2_noti_get_block(port, &noti);
      assert(error == RMA2_SUCCESS);
 
      num = rma2_noti_get_lock_number(noti);
      result = rma2_noti_get_lock_result(noti);
      value = rma2_noti_get_lock_value(noti);
      tmp = lock_increment*((lock_value/lock_increment)+1);
      assert(num == lock_number);

      if(i==0) { //requester notification
        assert(result == 0);
        assert(value == tmp);
      } else { //other notifications
        assert(result == 1);
        assert(value == tmp-lock_increment);
      }

      error = rma2_noti_free(port, noti);
      assert(error == RMA2_SUCCESS);
  }

  error = rma2_disconnect(port, handle);
  if(error) {
    printf("disconnect error; %i\n", error);
    abort();
  }

  error = rma2_close(port);
  if(error) {
    printf("close: error %i", error);
    abort();
  }

  printf("Finished RMA lock test without errors\n.");

  return 0;
}
