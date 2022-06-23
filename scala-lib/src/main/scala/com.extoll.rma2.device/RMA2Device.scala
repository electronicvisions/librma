/**
 *
 */
package com.extoll.rma2.device

import uni.hd.cag.osys.rfg.rf.device._
import org.bridj.Pointer
import com.extoll.rma2.Rma
import com.extoll.rma2.DummyRmaNode
import com.extoll.rma2.RMA2_Endpoint
import com.extoll.rma2.pinned_buffer
import com.extoll.rma2.Rma2Library

/**
 * @author rleys
 *
 */
class RMA2Device {

}

/**
 * Companion object to open/close the native interface
 */
object RMA2Device extends Device {

  /**
   * This is the global RMA endpoint
   */
  var endpoint: Pointer[RMA2_Endpoint] = null

  /**
   * Connections pool
   * Maps nodeId to opened Connection
   */
  var connections = Map[Short, Rma#RMAConnection]()

  /**
   * This is the pinned buffer used for read/write interactions
   */
  var memoryRegion = new pinned_buffer

  var rma = new Rma

  /**
   * This is the RMA2 registered memory
   */
  //var rma2Region : Pointer[RMA2_Region] = null

  /**
   * Opens the RMA Device
   */
  def open() = {

    require(endpoint == null)

    rma.open

    /*
    // Open RMA Device
    //---------------------

    // Prepare pointer on pointer to get allocated results from library
    var endpointPtr = Pointer.allocatePointer(classOf[RMA2_Endpoint])
    var res = Rma2Library.rma2_open(endpointPtr);

    if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS ) {
      throw new RuntimeException("Could not open RMA2 device. Is the RMA2 device present? The driver loaded?")
    }

    // Ok save endpoint
    this.endpoint = endpointPtr.get()

    // Prepare pinned Buffer
    //----------------------------
    //this.memoryRegion.buffer(Pointer.allocatePointer(classOf[Long]))
    this.memoryRegion.size(4096)
    BackendLibrary.setup_physical_buffer(Pointer.pointerTo(this.memoryRegion));
    if (this.memoryRegion.valid()!=1) {
      throw new RuntimeException("Could not allocate pinned memory")
    }
    //println(s"Pinned memory at: 0x${(this.memoryRegion.physical_buffer() & 0x7FFFFFFFFFFFFFFFL).toHexString}")
    //println(s"Pinned memory at: 0x${(this.memoryRegion.physical_buffer() & 0x00000000FFFFFFFFL).toHexString}")
    println(s"Pinned memory at: 0x${(this.memoryRegion.physical_buffer()).toHexString}")

    //-- Register pinned Buffer for interactions.
    //-----------------------------------------------
	/*var regionPtrPtr = Pointer.allocatePointer(classOf[RMA2_Region])
	var bufferSize = 1024;
	var buffer = Pointer.allocateCLongs(bufferSize)

	//res = Rma2Library.rma2_register(endpoint,this.memoryRegion.buffer(),this.memoryRegion.size(),regionPtrPtr)
	res = Rma2Library.rma2_register(endpoint,buffer,bufferSize,regionPtrPtr)
	if (res != RMA2_ERROR.RMA2_SUCCESS ) {
		throw new RuntimeException(s"Could not register memory to RMA2: ${res.value()}")
	}*/
	//this.rma2Region = regionPtrPtr.get();
*/
  }

  /**
   * Closes the Endpoint
   */
  def close() = {

    require(endpoint != null)

    // Close all Connections
    //-----------------------
    this.connections.values.foreach(_.close)
    this.connections.empty

    // Close RMA
    //----------------
    var res = Rma2Library.rma2_close(this.endpoint)
    if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS) {
      throw new RuntimeException("En error occured while closing RMA2 device")
    }
    this.endpoint = null

  }

  /**
   * Open Connection to RMA
   *
   */
  /* def openConnection(nodeId:Short) : Pointer[RMA2_Connection] = {

    require(endpoint!=null)

    //-- Connection handler is allocated by connect
    var connectionHandlePtrPtr = Pointer.allocatePointer(classOf[RMA2_Connection])
    var connectionOptions = Rma2Library.RMA2_Connection_Options.RMA2_CONN_PHYSICAL.value()|Rma2Library.RMA2_Connection_Options.RMA2_CONN_RRA.value()

    //-- Connect
    var vpid : Short = 0
    var res = Rma2Library.rma2_connect(this.endpoint,
    							nodeId,vpid,
    							Rma2Library.RMA2_Connection_Options.fromValue(connectionOptions.toInt),
    							connectionHandlePtrPtr)
    if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS ) {
      throw new RuntimeException(s"Could not open connection to node $nodeId: $res")
    }

    println(s"Connected to $nodeId")

    // Return
    connectionHandlePtrPtr.get()

  }*/

  /**
   * Returns the node id of the local device
   */
  def getNodeId: Short = Rma2Library.rma2_get_nodeid(this.endpoint);

  def getConnection(nodeId: Short): Rma#RMAConnection = {
    this.connections.contains(nodeId) match {
      case true => this.connections(nodeId)
      case false =>

        // Create Connection
        //---------------------
        var dummyNode = new DummyRmaNode(nodeId)
        var rmaConnection = rma.createConnection(dummyNode)
        rmaConnection.setPhysicalMode
        rmaConnection.open

        this.connections += (nodeId -> rmaConnection);
        this.connections(nodeId)
    }
  }

  /**
   * Device trait implementation
   */
  def readRegister(nodeId: Short, address: Long): Option[Long] = {

    // Get/Open Connection
    //--------------------------
    var connection = getConnection(nodeId)

    // Perform Read
    //----------------------
    Some(connection.readLong(address))

    //////////////
    // OLD
    ///////////////////////////

    /*
      //-- Get the Network address of the region index 0 (not pooling for now)
	   //var regionNLA = Pointer.allocateLong()
	   //Rma2Library.rma2_get_nla(this.rma2Region,0,regionNLA);

	     println(s"Performing read at 0x${address.toHexString}");

    	//-- Perform a read
    	var res = Rma2Library.rma2_post_get_bt_direct(endpoint,
    							connection,
    							(this.memoryRegion.physical_buffer()),
    							8,
    							// Address
    							address,
    							// Specs
    							Rma2Library.RMA2_Notification_Spec.RMA2_COMPLETER_NOTIFICATION,
    							Rma2Library.RMA2_Command_Modifier.RMA2_CMD_DEFAULT)

    	if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS ) {
    		throw new RuntimeException(s"Could not perform read: $res")
    	}

    	//-- Block on notification to wait for read to be done

    	// Do
    	var notificationPtrPtr = Pointer.allocatePointer(classOf[RMA2_Notification])
    	res = Rma2Library.rma2_noti_get_block(this.endpoint,notificationPtrPtr)
    	if (res != Rma2Library.RMA2_ERROR.RMA2_SUCCESS ) {
    		throw new RuntimeException(s"Blocking on read notification failed: $res")
    	}

    	Rma2Library.rma2_noti_dump(notificationPtrPtr.get());


    	// Free notification
    	Rma2Library.rma2_noti_free(this.endpoint, notificationPtrPtr.get());

    	// Return
    	//----------
    	for ( i <- 0 to 7) {
    	  println(s"Buffer content at $i : ${this.memoryRegion.buffer.get(i)}")
    	}

    	/*for ( i <- 0 to ((this.memoryRegion.size()/8)-1).toInt) {
    	  if (this.memoryRegion.buffer().get(i) > 0) {
    	    println(s"Found non 0 value: ${this.memoryRegion.buffer().get(i)}")
    	  }
    	}*/
    	/*this.memoryRegion.buffer().get().toArray().filter( value => value > 0 ).foreach {

    	  value => println(s"Found non 0 value: $value")

    	}*/

    	Option[Long](this.memoryRegion.buffer().get(0))
	*/

  }

  def writeRegister(nodeId: Short, address: Long, value: Long) = {

    // Get/Open Connection
    //--------------------------
    var connection = getConnection(nodeId)

    // Perform write
    //---------------------
    connection.writeBlocking(address,value)

  }

}
