package com.extoll.rma2

import java.nio.ByteBuffer
import com.idyria.osi.tea.logging.TLogSource
import org.bridj.Pointer
import java.nio.IntBuffer
import java.util.concurrent.locks.ReentrantLock
import com.idyria.osi.tea.listeners.ListeningSupport
import com.idyria.osi.tea.thread.ThreadLanguage
import java.nio.ByteOrder
import java.util.concurrent.TimeUnit
import java.util.concurrent.TimeoutException
 
/**
 * Modelises an RMA Interface with endpoint request, connections etc..
 */
class Rma extends ThreadLanguage with TLogSource {

  /**
   * Connections pool
   * Maps nodeId to opened Connection
   */
  var connections = Map[Short, Pointer[RMA2_Connection]]()

  // Init: Try to open RMA
  //-------------------------------

  /**
   * The Endpoint represents the RMA library local interface
   */

  var endpoint: Pointer[RMA2_Endpoint] = null

  /**
   * Open Local endpoint
   */
  def open() = {

    // Open
    //-------------------
    var endpointPtr = Pointer.allocatePointer(classOf[RMA2_Endpoint])
    var res = Rma2Library.rma2_open(endpointPtr);

    if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS) {
      throw new RuntimeException("Could not open RMA2 device. Is the RMA2 device present? The driver loaded?")
    }

    // Store Endpoint reference
    //-------------------
    this.endpoint = endpointPtr.get

    // Start Notification monitoring thread
    //-----------------
    //this.notificationThread.setName("Notification Polling Thread")

  }

  /**
   * Close Endpoint and all connections
   */
  def close() = {
    this.endpoint match {
      case null ⇒
      case _ ⇒
        Rma2Library.rma2_close(this.endpoint)
        this.endpoint = null
        this.connections = this.connections.empty
    }
  }

  /**
   * Returns local VPID
   */
  def getVPID: Short = {
    require(endpoint != null, "RMA not opened")
    Rma2Library.rma2_get_vpid(endpoint)

  }
 /**
   * Returns local NodeID
   */
  def getNodeId: Int = {
    require(endpoint != null, "RMA not opened")
    Rma2Library.rma2_get_nodeid(endpoint)

  }
  // Notifications
  //-------------------

  // Use a mutable collection here to avoid copies, because the content May have a lot of objects
  val notifications = scala.collection.mutable.HashMap[RmaNotification.Type.Value, java.util.concurrent.LinkedBlockingQueue[RmaNotification]]()

  // Init queues
  notifications += (RmaNotification.Type.REQUESTER -> new java.util.concurrent.LinkedBlockingQueue[RmaNotification]())
  notifications += (RmaNotification.Type.RESPONDER -> new java.util.concurrent.LinkedBlockingQueue[RmaNotification]())
  notifications += (RmaNotification.Type.COMPLETER -> new java.util.concurrent.LinkedBlockingQueue[RmaNotification]())

  //-- Thread that will poll on notifications upon request
  /* val notificationThread = createThread {

    println(s"NOtification thread started")
    while (true) {

      // Wait for a notification
      //---------------------------------
      var notificationPtrPtr = Pointer.allocatePointer(classOf[RMA2_Notification])

      Rma2Library.rma2_noti_get_block(endpoint, notificationPtrPtr) match {
        case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
        case res ⇒
          throw new RuntimeException(s"An error occured while waiting for a notification: $res")
      }
      
      
      var notification = Rma2Library.rma2_noti_get_notification_type(notificationPtrPtr.get) match {
	    case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION) =>
	       RmaNotification(RmaNotification.Type.REQUESTER)
	    case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_RESPONDER_NOTIFICATION) =>
	       RmaNotification(RmaNotification.Type.RESPONDER)
	    case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_COMPLETER_NOTIFICATION) =>
	       RmaNotification(RmaNotification.Type.COMPLETER)
	  }
      

     /* //-- Save the notification
      var savedNotification = Pointer.allocate(classOf[RMA2_Notification])
      Rma2Library.rma2_noti_dup(notificationPtrPtr.get, savedNotification)*/

      //-- Free
      Rma2Library.rma2_noti_free(endpoint, notificationPtrPtr.get());
      //notificationPtrPtr.get().release()
      notificationPtrPtr.release()
      
      // Record it to the matching queue
      //--------------------
     // var notification = RmaNotification(savedNotification)

     // println(s"Offering Notification to queue: " + notification.notificationType+ s"(size: ${notifications(notification.notificationType).size()})")

      //-- Try to offer, and fail if not working
      notifications(notification.notificationType).offer(notification,1000, TimeUnit.MILLISECONDS) match {
        case true => 
          println(s"Offered Notification to queue: " + notification.notificationType+ s"(size: ${notifications(notification.notificationType).size()})")

        case false => 
          
          throw new RuntimeException(s"Could not offer ${notification.notificationType} notification to queue because it seems to be full")
      }

      //Rma2Library.rma2_noti_dump(notificationPtrPtr.get());

    }
  }
  notificationThread.setDaemon(true)*/

  /**
   * Waits blocking for a notification, frees it, and returns
   *
   *
   *
   */
  def waitForNotification(notificationType: RmaNotification.Type.Value): RmaNotification = {

      // Try to get from Queue first
      //------------------
      notifications(notificationType).poll() match {
          case null =>

              // Loop search for notification, because waited one could be overriden by others
              var foundNotification: Option[RmaNotification] = None
              val startTime = System.currentTimeMillis()
              val maxWaitTime = 200L

              while (foundNotification == None) {

                  // Wait for a notification
                  //---------------------------------
                  var notificationPtrPtr = Pointer.allocatePointer(classOf[RMA2_Notification])

                  //logInfo[Rma]("Waiting for notification")
                  val gotNoti = Rma2Library.rma2_noti_probe(endpoint, notificationPtrPtr) match {
                      case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒ true
                      case res if (res == Rma2Library.RMA2_ERROR.RMA2_NO_NOTI) => false
                      case res ⇒
                              throw new RuntimeException(s"An error occured while waiting for a notification: $res")
                  }

                  if(gotNoti) {
                      var notification = Rma2Library.rma2_noti_get_notification_type(notificationPtrPtr.get) match {
                          case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION) =>
                              RmaNotification(RmaNotification.Type.REQUESTER)
                          case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_RESPONDER_NOTIFICATION) =>
                              RmaNotification(RmaNotification.Type.RESPONDER)
                          case t if (t == Rma2Library.RMA2_Notification_Spec.RMA2_COMPLETER_NOTIFICATION) =>
                              RmaNotification(RmaNotification.Type.COMPLETER)
                      }
                      //logInfo[Rma]("Got NOtification: " + notification.notificationType)

                      //-- Save the notification
                      /*var savedNotification = Pointer.allocate(classOf[RMA2_Notification])
            Rma2Library.rma2_noti_dup(notificationPtrPtr.get, savedNotification)*/

                      //-- Free
                      Rma2Library.rma2_noti_free(endpoint, notificationPtrPtr.get());
                      //notificationPtrPtr.get().release()
                      notificationPtrPtr.release()

                      //-- Create
                      //var notification = RmaNotification(savedNotification)

                      // If it is the execpted notification, return it
                      //---------------
                      if (notification.notificationType == notificationType) {
                          foundNotification = Some(notification)
                          logFine[Rma](s"Found Searched notification")
                      } // Otherwise Record it to the matching queue
                      //--------------------
                      else {

                          notifications(notification.notificationType).offer(notification, 1000, TimeUnit.MILLISECONDS) match {
                              case true =>
                                  logFine[Rma](s"Offered Notification to queue: " + notification.notificationType + s"(size: ${notifications(notification.notificationType).size()})")

                              case false =>

                                  throw new RuntimeException(s"Could not offer ${notification.notificationType} notification to queue because it seems to be full")
                          }

                      }
                  } else {
                      val currentTime = System.currentTimeMillis()
                      if (currentTime - startTime > maxWaitTime) {
                          throw new TimeoutException(s"Waitet longer than ${maxWaitTime}ms for a ${notificationType} notification")
                      }
                  }

              }
              // EOF Loop
              foundNotification.get

          case notification => notification
      }

    // var notification = RmaNotification(savedNotification)

    // println(s"Offering Notification to queue: " + notification.notificationType+ s"(size: ${notifications(notification.notificationType).size()})")

    //-- Try to offer, and fail if not working
    /*notifications(notification.notificationType).offer(notification, 1000, TimeUnit.MILLISECONDS) match {
      case true =>
        println(s"Offered Notification to queue: " + notification.notificationType + s"(size: ${notifications(notification.notificationType).size()})")

      case false =>

        throw new RuntimeException(s"Could not offer ${notification.notificationType} notification to queue because it seems to be full")
    }*/

    // Take blocking from notification queue
    //----------------------
    /* notifications(notificationType).poll(5, TimeUnit.SECONDS) match {
      case null =>

        throw new RuntimeException(s"Waited for 5 seconds on ${notificationType} notification, but nothing came back: size:  ${notifications(notificationType).size()}")
      case n => n
    }*/

  }

  // Memory Buffers
  //---------------------

  /**
   * Size in bytes
   */
  class RmaRegion(var size: Int) extends TLogSource {

    var bytes: ByteBuffer = null

    /**
     * Set after allocate
     */
    var regionPointer: Pointer[RMA2_Region] = null

    var physicalBuffer: pinned_buffer = null

    var physical = false

    /**
     * Registers the Byte Buffer
     */
    def register = {

      physical match {

        //-- Allocate Physial Buffer
        case true =>

          this.physicalBuffer = new pinned_buffer
          this.physicalBuffer.size(size)
          PinnedbufferLibrary.setup_physical_buffer(Pointer.pointerTo(this.physicalBuffer));
          if (this.physicalBuffer.valid() != 1) {
            throw new RuntimeException("Could not allocate pinned memory")
          }

        //-- Allocate Registered User Space Buffer
        case false =>

          bytes = ByteBuffer.allocateDirect(size)
          bytes.order(ByteOrder.LITTLE_ENDIAN)

          //-- Register
          regionPointer = Pointer.pointerTo(new RMA2_Region)
          Rma2Library.rma2_register_nomalloc(endpoint, Pointer.pointerToBytes(bytes), size, regionPointer) match {
            case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
            case res ⇒
              throw new RuntimeException(s"Could not register allocated memory with ByteBuffer.allocate, using rma2_register_nomalloc: $res")
          }
      }

    }

    /**
     * Unregister and release memory
     */
    def release = {

      this.physical match {
        case true =>

          PinnedbufferLibrary.release_physical_buffer(Pointer.pointerTo(this.physicalBuffer))

        case false =>

          require(regionPointer != null, "Register memory before relasing it!")
          Rma2Library.rma2_unregister_nofree(endpoint, regionPointer)
          regionPointer.release()
          bytes = null
          regionPointer = null

      }

    }

    /**
     * Returns the address value of the currently registered buffer, which can be used for remote senders to target this region
     */
    def getNLA: Long = {

      require(physical == false, "NLA is only available for registered User Space Buffers")

      var ptr = Pointer.allocate(classOf[java.lang.Long])
      Rma2Library.rma2_get_nla(regionPointer, 0, ptr) match {
        case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
        case res ⇒
          throw new RuntimeException(s"Could not get NLA of RmaRegion, maybe allocation failed previously: $res")
      }

      ptr.get()

    }

    /**
     * Returns the Physical Address of physical buffer
     */
    def getPhysicalAddress: Long = {

      require(physical == true, "getPhysicalAddress is only available for physical allocated buffers")

      this.physicalBuffer.physical_buffer()
    }

    /**
     * Returns the NLA or the Physical Buffer address of the buffer
     */
    def getAddress: Long = {
      this.physical match {
        case true =>
          this.getPhysicalAddress
        case false =>
          this.getNLA
      }
    }

    // Put/Read Functions
    //------------------------

    def put(byteAddress: Int, value: Long) = {
      this.physical match {
        case true =>
          require(this.physicalBuffer != null, "Cannot put to non initialised memory region")

          this.physicalBuffer.buffer().setLongAtOffset(byteAddress, value)

        case false =>

          this.bytes.putLong(byteAddress, value)

      }
    }

  } // EOF RMA Region

  /**
   * size is in bytes
   * @return Registered RmaRegion
   */
  def allocate(size: Int): RmaRegion = {

    //-- Allocate Region
    var region = new RmaRegion(size)
    region.register

    return region

  }

  /**
   * Allocate a Receive region located in physical address space
   */
  def allocatePhysical(size: Int): RmaRegion = {

    var region = new RmaRegion(size)
    region.physical = true

    region.register

    return region

  }

  // Connection to another Node
  //------------------------------------
  class RMAConnection(var targetNode: RmaNode, var vpid: Short = 0) extends TLogSource with ThreadLanguage with ListeningSupport {

    var connection: Pointer[RMA2_Connection] = null

    /**
     * Connection Options per default: Default
     */
    var connectionOptions = Rma2Library.RMA2_Connection_Options.RMA2_CONN_DEFAULT.value
    var physical = false

    /**
     * Small read/writes buffer
     */
    var singleOperationsRegion: Rma#RmaRegion = null

    /**
     * Change connection options to Physical and RRA
     */
    def setPhysicalMode = {

      connection match {
        case null =>
          this.physical = true
          this.connectionOptions = Rma2Library.RMA2_Connection_Options.RMA2_CONN_PHYSICAL.value() | Rma2Library.RMA2_Connection_Options.RMA2_CONN_RRA.value()
        case _ => throw new RuntimeException(s"Requesting connection options setup,  but a connection is openened. Ensure you first cleaned the RMA connection before changing connection options, then reopen the connection")
      }

    }
    /**
     * Open connection to RMA for target Node ID
     */
    def open: Unit = {

      //-- Connection handler is allocated by connect
      var connectionHandlePtrPtr = Pointer.allocatePointer(classOf[RMA2_Connection])

      //-- Connect
      var vpid: Short = 0
      var res = Rma2Library.rma2_connect(endpoint,
        targetNode.id, vpid,
        Rma2Library.RMA2_Connection_Options.fromValue(connectionOptions.toInt),
        connectionHandlePtrPtr)
      if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS) {
        throw new RuntimeException(s"Could not open connection to node ${targetNode.id}: $res")
      }

      //-- Create single operations buffer
      this.singleOperationsRegion = this.physical match {
        case true  => allocatePhysical(4096)
        case false => allocate(4096)
      }

      logInfo[Rma](s"Opened connection to node id " + targetNode.id)
      //println(s"Connected to $nodeId")

      // Return
      this.connection = connectionHandlePtrPtr.get()

    }

    def close = {
      require(this.connection != null, "Connection must be opened to be closed")

    }

    // Read
    //---------------

    /**
     * Read a long from this connection at a specific address
     */
    def readLong(address: Long): Long = {

      //-- Allocate Receive Region
      /* var receiveRegion = this.physical match {
        case true  => allocatePhysical(4096)
        case false => allocate(8)
      }*/
      var receiveRegion = this.singleOperationsRegion

      //-- Perform a read
      var res = Rma2Library.rma2_post_get_bt_direct(endpoint,
        connection,
        receiveRegion.getAddress,
        8,
        // Address
        address,
        // Specs
        Rma2Library.RMA2_Notification_Spec.RMA2_REQCOMPL_NOTIFICATION,
        Rma2Library.RMA2_Command_Modifier.RMA2_CMD_DEFAULT)

      if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS) {
        throw new RuntimeException(s"Could not perform readLong: $res")
      }

      //-- Block on notification to wait for read to be done
        try {
            waitForNotification(RmaNotification.Type.REQUESTER)
            waitForNotification(RmaNotification.Type.COMPLETER)
        } catch {
            case t: TimeoutException =>
                throw new TimeoutException(t.getMessage + s" while reading from address $address on node id ${targetNode.id}")
        }

      //-- Copy Result, Release buffer and  Return
      var result = this.physical match {
        case true  => receiveRegion.physicalBuffer.buffer().get(0).toLong
        case false => receiveRegion.bytes.asLongBuffer().get()
      }

      //receiveRegion.release

      result
    }

    // Send Functions
    //---------------------

    /**
     * Writes Blocking one Long to an address
     */
    def writeBlocking(targetAddress: Long, long: Long): Unit = {

      // Allocate Send Buffer & Copy Value to buffer
      //------------------------
      /*var sendRegion = this.physical match {
        case true  => allocatePhysical(4096)
        case false => allocate(8)
      }*/
      var sendRegion = this.singleOperationsRegion

      // Copy Value to buffer
      //---------------
      sendRegion.put(0, long)

      // Send Blocking
      //--------------------
      Rma2Library.rma2_post_put_bt_direct(endpoint,
        connection,
        sendRegion.getAddress,
        8,
        targetAddress,
        Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION,
        Rma2Library.RMA2_Command_Modifier.RMA2_CMD_DEFAULT) match {
          case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
          case res ⇒ throw new RuntimeException("Could not send Long ")
        }

      waitForNotification(RmaNotification.Type.REQUESTER)

      // Release and Return
      //---------------------
      //sendRegion.release

    }

    /**
     * Send the provided Array of ints, using as much as RMA send as required
     * The targetNLA can be provided
     *
     * The methods waits for a notification after each put, in single blocking mode
     *
     * @param notificationsPostpone (notificationsPostpone-1) packets are sent without waiting on the notification, the notification will happen at the end of the send loop
     *
     */
    def writeBlocking(ints: Array[Int], targetNLA: Long, packetSize: Int = Rma.packetSize, notificationsPostpone: Int = 0): Unit = {

      // Send Grouped by the number of RMA send required to send all the provided ints
      //-------------------

      //-- Group of bytes must be multiple of QuadWord, which makes two ints (2* 4 bytes)
      var divideby = 4 // bytes usage
      var intsPerPacket = (packetSize / divideby).floor.toInt
      var totalPackets = (ints.length.toDouble / intsPerPacket.toDouble).ceil.toInt

      logInfo[Rma](s"Starting send $totalPackets for ${ints.length}, per packet: $intsPerPacket ")

      var packetContentPtr = Pointer.pointerToInts(IntBuffer.wrap(ints))
      var registeredPacketContentPtrPtr = Pointer.allocatePointer(classOf[RMA2_Region])

      //-- Register
      Rma2Library.rma2_register(endpoint, packetContentPtr, ints.length * 4, registeredPacketContentPtrPtr) match {
        case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
        case res ⇒ throw new RuntimeException(s"Could not register Integer Packet content as buffer to RMA: $res")
      }

      //-- Get RMA Region pointer
      var registeredPacketContentPtr = registeredPacketContentPtrPtr.get.as(classOf[RMA2_Region])

      //-- Send By looping over packetCounts, and incrementing offset
      var seenNotifications = 0
      for (i <- 0 to (totalPackets - 1)) {

        var noti = Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION.value()

        // Size to send
        //------------------
        var intsToSend = (((i * intsPerPacket) + intsPerPacket)) match {

          //-- Overreach size
          case total if (total > ints.size) =>

            //println(s"last send, should be waiting for notification")

            intsPerPacket - (total - ints.size)

          //-- max
          case _ =>

            // println(s"Not last send")
            intsPerPacket
        }

        // Last send
        //------------------
        /*if (i == (totalPackets - 1)) {
          noti = Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION.value()
        }*/

        // println(s"Writing RMA packet with ${intsToSend} ints ($i/${totalPackets})")

        // println(s"Requester Noti value: "+Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION.value())

        // Rma2Library.RMA2_Notification_Spec.fromValue((Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION.value() | Rma2Library.RMA2_Notification_Spec.RMA2_COMPLETER_NOTIFICATION.value()))

        var offset = (i * intsPerPacket) * 4
        var size = intsToSend * 4

        //println(s"Write from $offset for a length of $size")

        //-- Send
        Rma2Library.rma2_post_put_bt(endpoint,
          connection,
          registeredPacketContentPtr,
          offset, size,
          targetNLA + offset,
          Rma2Library.RMA2_Notification_Spec.fromValue(noti.toInt),
          Rma2Library.RMA2_Command_Modifier.RMA2_CMD_DEFAULT) match {
            case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
            case res ⇒ throw new RuntimeException("Could not send Integer Packet content: " + i)
          }

        // Wait for notification for this send based on the notification limit
        // - As long as i < firstNotificationLimit, don't wait for notification
        //-------------------
        if (i >= notificationsPostpone - 1) {
          waitForNotification(RmaNotification.Type.REQUESTER)
          seenNotifications += 1
        }

        //-- Progress
        var progress: Double = (((i * intsPerPacket) + intsToSend).toDouble / (ints.size).toDouble) * 100
        @->("write.progress", progress)
        logFine[Rma](s"Progress ($i / $totalPackets): ")
      }
 
      // Wait for remaining notifications, i.e: count from 1 to firstNotificationLimit-1 (because of >= before)
      //----------
      var remainingNotifications = (totalPackets) - seenNotifications
      if (remainingNotifications >= 1) {
        for (i <- 1 to remainingNotifications) {
          logFine[Rma](s"Waiting for remaining notification $i/$remainingNotifications")
          waitForNotification(RmaNotification.Type.REQUESTER)
        }
      } 

      //-- Unregister memory
      logInfo[Rma](s"Done sending, releasing")
      //sendRegion.release
      registeredPacketContentPtr.release()
      Rma2Library.rma2_unregister_nofree(endpoint, registeredPacketContentPtr)

      ////////////////////////////////
      // Old Method
      ////////////////////////////////
      /*
      var groupedList = ints.grouped(intsPerPacket).toArray

      for (i <- 0 to (groupedList.size - 1)) {

        //var groupedWithIndex = groupedList.zipWithIndex 

        // logInfo[Rma](s"Splitting writes in ${groupedList.size} groups, index size: ${groupedWithIndex.size}")

        // groupedWithIndex.foreach {

        //case (packetContent,i) ⇒
        var packetContent = groupedList(i)

        // If Packet content is not a multiple of QuadWords, we should pad
        /* var packetContent = ((packetContentBase.length*4) % 8) match {
          //-- All fine
          case 0 => packetContentBase
          //-- Pad with 8-res extra ints
          case res if(res==4) => 
            
            logInfo[Rma](s"There are $res bytes remaining to be QW aligned")
            
            //-- Create a Bigger buffer for the extra Ints, copy existing to it, and add required remaining Ints
            var buffer = IntBuffer.allocate(packetContentBase.length+1)
            buffer.put(packetContentBase)
            buffer.put(0)
            buffer.flip
            
            var arr = buffer.array()
            
            logInfo[Rma](s"After Padding, the resulting array to write has a length of ${arr.length*4} bytes")
            
            arr
          case res => 
            throw new RuntimeException(s"Write Int of ${packetContentBase} should be padding with $res bytes, which is not so normal, as we should be padding with only one Integer...if not one integer, we should allready be aligned")
        }*/

        // Send
        //---------------
        logInfo[Rma](s"Writing RMA packet with ${packetContent.length} ints (${packetContent.length * 4}) ($i/${groupedList.size})")

        //-- Register content as Buffer   
        var packetContentPtr = Pointer.pointerToInts(IntBuffer.wrap(packetContent))
        var registeredPacketContentPtrPtr = Pointer.allocatePointer(classOf[RMA2_Region])

        /* logInfo[Rma](s"Pointer to buffer address: " + packetContentPtr.getPeer().toHexString)
          logInfo[Rma](s"Pointer to buffer valid bytes: " + packetContentPtr.getValidBytes())
          logInfo[Rma](s"First int is: " + packetContentPtr.getInt().toHexString)

          */
        //-- Register
        Rma2Library.rma2_register(endpoint, packetContentPtr, packetContent.length * 4, registeredPacketContentPtrPtr) match {
          case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
          case res ⇒ throw new RuntimeException(s"Could not register Integer Packet content as buffer to RMA: $res")
        }

        //-- Get RMA Region pointer
        var registeredPacketContentPtr = registeredPacketContentPtrPtr.get.as(classOf[RMA2_Region])

        //logInfo[Rma](s"Registered Region has a size of: ${registeredPacketContentPtr.get.size()}, start is: ${registeredPacketContentPtr.get.start().getLong().toHexString}")

        //-- Send
        Rma2Library.rma2_post_put_bt(endpoint,
          connection,
          registeredPacketContentPtr,
          0, packetContent.length * 4,
          targetNLA,
          Rma2Library.RMA2_Notification_Spec.fromValue((Rma2Library.RMA2_Notification_Spec.RMA2_REQUESTER_NOTIFICATION.value() | Rma2Library.RMA2_Notification_Spec.RMA2_COMPLETER_NOTIFICATION.value()).toInt),
          Rma2Library.RMA2_Command_Modifier.RMA2_CMD_DEFAULT) match {
            case res if (res == Rma2Library.RMA2_ERROR.RMA2_SUCCESS) ⇒
            case res ⇒ throw new RuntimeException(s"Could not send Integer Packet content: $res")
          }

        //-- Wait For Put to be completed
        try {
          var notification = waitForNotification(RmaNotification.Type.REQUESTER)
        } finally {

          //-- Unregister memory
          registeredPacketContentPtr.release()
          Rma2Library.rma2_unregister_nofree(endpoint, registeredPacketContentPtr)

        }

      }*/

    } // EOF Write blocking

  }

  /**
   * Create Connection, but don't open it
   */
  def createConnection(targetNode: RmaNode): RMAConnection = {

    require(endpoint != null, "RMA Must be openened before creating a connection")

    var conn = new RMAConnection(targetNode)

    return conn

  }

  /**
   * Open Connection to RMA for a specific target Node ID
   *
   */
  def openConnection(targetNode: RmaNode): RMAConnection = {

    require(endpoint != null, "RMA Must be openened before opening a connection")

    var conn = new RMAConnection(targetNode)
    conn.open

    return conn

  }

  // Notifications
  //--------------------

  /**
   * Size must be a multiple of a page: 4096Kb, and a power of tow
   */
  def setSegmentSize(size: Int) = {

    require(endpoint != null, "RMA Must be openened")

    //-- Multiple of 4K
    (size % (4096)) match {

      //-- Ok
      case 0 =>
        Rma2Library.rma2_set_queue_segment_size(endpoint, size)

      //-- Not Ok
      case notok =>
        throw new RuntimeException(s"Segement size must be multiple of a 4KB page size")

    }

  }

  def getQueueSegementSize: Int = {
    Rma2Library.rma2_get_queue_segment_size(endpoint)
  }

  /**
   * Set the notification buffer Size, i.e count * (size of notification)
   * The final size is saled to be a multiple of the segment size, which is a requirement of the RMA API
   *
   * @return the Notification queue size set by this function (not the number of notifications, but the numberofnotifications * notification size that has been set)
   */
  def setSupportedNotificationsCount(count: Int) = {

    require(endpoint != null, "RMA Must be openened")

    //-- Queue size must be a multiple of page size
    // var queueSize = ((count * Rma.notificationSize) / 4096).ceil.toInt

    //-- Size is at least one page, and a certain number of pages
    var queueSize = 4096 + (4096 * ((count * Rma.notificationSize) / 4096).floor.toInt)

    println(s"Segement size is: " + getQueueSegementSize)
    println(s"Queue size is: " + queueSize)

    //-- Set size to a multiple of the segment size
    var res = Rma2Library.rma2_set_queue_size(endpoint, queueSize)

    //-- Check Result
    if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS) {
      throw new RuntimeException(s"Could not set Notifications queue size: $res")
    }

    //-- Start notification thread
    //this.notificationThread.start

    queueSize
  }

}
object Rma {

  val packetSize = 240 // bytes
  val notificationSize = 16 // bytes

}
