package com.extoll.rma2


import org.bridj.Pointer
   
/**
 * This is a wrapper around a Notification Pointer
 */
/*class RmaNotification(var notification: Pointer[RMA2_Notification]) {

  //-- Init with type
  var notificationType = Rma2Library.rma2_noti_get_notification_type(notification) match {
    case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION) =>
      RmaNotification.Type.REQUESTER
    case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_RESPONDER_NOTIFICATION) =>
      RmaNotification.Type.RESPONDER
    case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_COMPLETER_NOTIFICATION) =>
      RmaNotification.Type.COMPLETER
  }
  */
class RmaNotification(var notificationType : RmaNotification.Type.Value) {
  // Accessor Functions
  //-----------------------

}
object RmaNotification {

 // def apply(p:Pointer[RMA2_Notification]) = new RmaNotification(p)
  
  def apply(t : RmaNotification.Type.Value) = new RmaNotification(t)
  
  object Type extends Enumeration {
    type Type = Value
    val REQUESTER, RESPONDER, COMPLETER = Value
  }

}