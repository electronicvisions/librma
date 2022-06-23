/***********************************************************************
*                                                                      *
* (C) 2008, Mondrian Nuessle, Computer Architecture Group,             *
* University of Heidelberg, Germany                                    *
* (C) 2011, Mondrian Nuessle, EXTOLL GmbH, Germany                     *
*                                                                      *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU Lesser General Public License as       *
* published by the Free Software Foundation; either version 2 of the   *
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

#ifndef LIBRMA2_H
#define LIBRMA2_H

/*!
  \file   rma2.h
  \brief  Header file for the EXTOLL RMA2 API, librma2

  See also \ref main.

  \author Dr. Mondrian Nuessle, EXTOLL GmbH, mondrian.nuessle@extoll.de
  \date   2011-10-07
*/

/*!
   \mainpage  RMA2 API Overview

   
   \author    Dr. Mondrian Nuessle, EXTOLL GmbH, mondrian.nuessle@extoll.de
   \date      2011-10-07
   \anchor    main
   \par

  \section INTRO Introduction

  LIBRMA2 is the API library for remote memory access in the EXTOLL network.
  RMA2 is also the name of the functional unit in the EXTOLL NIC which
  performs the RMA2 operations.

  LIBRMA2 is a library that allows user-space level access to the hardware
  ressources of the EXTOLL RMA2 FU. It is not a full-fledged message-passing
  library. In the typical use-case, end-users use a higher level software 
  abstraction like MPI or UPC. Nevertheless, it is possible to employ librma2
  directly in applications, for example to leverage all of the hardware 
  functions or reach especially high performance.

  LIBRMA2 offers a number of services to upper-level-protocols (ULPs):
  - port management (i.e. opening and closing of an end-point)
  - connecting to remote peers
  - memory region management (i.e. registering and un-registering of regions, query regions)
  - command issue
  - notification handling
  - error handling

   \section CS Coding Style

   \subsection IDENTIFIER Identifier
   - All external viewable functions are prefixed with "rma2_". 
   - Internal functions are prefixed with "__RMA2_". 
   - Type names are prefixed with "RMA2_" respectively "__RMA2_". 
   - Words within one identifier are divided by underscores.
   - All words in function, variable and field names are small caps, 
     all words of a type identifier are capitalized.
   - Constants are completely capitalized

   \subsection FUNCARGS Function arguments
   - The first argument to all LIBRMA2 functions is of type RMA2_Port 
   - All input parameters come before output parameters.
   - All output parameters are pointers. 
   - (Nearly) All functions return an error code of type RMA2_ERROR.

   \section MM Memory management and valid pointers

   Almost no function of LIBRMA2 performs any tests on pointers specified being valid.
   It is therefore necessary that user applications ensure that pointers that are
   passed on to LIBRMA2 are valid.  If your application SEGFAULTS, check if all pointers passed to
   LIBRMA2 are valid.

   Internal memory is always allocated and deallocated by LIBRMA2 without user application
   intervention. In normal operation it is necessary to provide memory for an rma2_port
   variable (which in essence is a pointer variable), an rma2_handle variable for every
   virtual connection to be used.

   \section KERNLER Kernel interface

   librma2 uses the kernel interface exported from urma2.ko, the EXTOLL user-space RMA2 kernel driver. Specifically the
   following items are used:
   - open the special file /dev/rma2 to open a new endpoint (i.e. allocate a VPID)
   - close the special file again when done
   - use the following ioctl's:
   \li RMA2_IOCTL_GET_NOTI_SIZE - returns the size of the notification area in bytes
   \li RMA2_IOCTL_SET_NOTI_WP - sets the notification write pointer
   \li RMA2_IOCTL_SET_NOTI_RP - sets the notification read-pointer
   \li RMA2_IOCTL_GET_NODEID - returns the nodeid
   \li RMA2_IOCTL_GET_VPID - returns the VPID
   \li RMA2_IOCTL_REGISTER_REGION - to register memory and
   \li RMA2_IOCTL_UNREGISTER_REGION to unregister a region again
   \li FIXME: Probably forgot some
   
   - mmap is used to map notification queue, read-pointer and requester pages into user-space memory. Let size be the \e size of the notification queue in bytes, then  
   \li offset <tt>0</tt> up to <tt>\e size</tt> to map the notification queue
   \li offset <tt>\e size</tt> to <tt>\e size + 4096</tt> to map the read pointer 
   \li offset <tt>(\e size + 4096) + 4k*4* dest_node * dest_vpid</tt> for a requester page (the factor 4 is to jump over the AT and Interrupt Bits in the requester address)
   
   \section EXAMPLE API Usage
   
   \subsection INIT Initialization
   The following text should give the reader a basic understanding on how librma2 can be used for communication purposes.
   The used example code can be found under examples/send_recv_example.c.
   This files compiles and shows how the API works.
   The first thing every process using RMA2 will have to do, is to open an endpoint (or port):
   \code
   [...]
   RMA2_Port port;
   RMA2_ERROR rc;
   [...]
   rc=rma2_open(&port);
   if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"RMA open failed (%d)\n",rc);
      abort();
    }
   \endcode
   
   After an enpoint could be succesfully acquired, the process will have to open up one or more virtual connections to talk to other processes. Note that this
   is really a virtual connection, as there is no "connection state" used, it is purely a software abstraction to make addressing and managing of mode of operation
   simpler.
   \code
   RMA2_Handle handle;
   uint16_t destination_node=17;
   uint16_t destination_vpid=42;
   [...]
   rc=rma2_connect(port, destination_node,destination_vpid,RMA2_CONN_DEFAULT, &handle);
   if (rc!=RMA2_SUCCESS)
   {
      fprintf(stderr, "Connect failed(%d).\n",rc);
      rma2_close(port);
      abort();
   }
   \endcode
   \subsection COMM Communication
   In this example, the process will talk to the process with the VPID 42 on node 17.
   Now, it is time to register some memory. Generally, RMA2 works with registered memory. Registering memory is an operation performed by the kernel driver
   on behalf of the process. The memory region is pinned, i.e. cannot be swapped anymore, and hardware translation tables are set-up. Note that there is also the
   possibility to work directly with physical addresses, which by default is only allowed for priviliged processes, as this opens severe security problems.
   \code
   char send_buffer[LARGE_NUMBER];
   RMA2_Region* send_region;
   [...]
   rc=rma2_register(port,send_buffer, size, &send_region);
   if (rc!=RMA2_SUCCESS)
    {
      fprintf(stderr,"Send Buffer registration failed (%d).\n",rc);
      rma2_close(port);
      abort();
    }
   \endcode
   Now that a buffer has been registered, the process can perform data movement operations, for example put operations.
   Here, we will transport all of the send_port to a remote address (that we need to know of course!) using a quadword put operation.
   \code
   int offset=0;
   RMA2_Notification* notip;
   RMA2_NLA dest_address;
   [...]
   rc=rma2_post_put_qw(port, handle, send_region, 
                    offset, size, dest_address,
		    RMA2_COMPLETER_NOTIFICATION | RMA2_REQUESTER_NOTIFICATION,
                    RMA2_CMD_DEFAULT);
   if (rc!=RMA2_SUCCESS)
   {
      fprintf(stderr,"Posting of put operation failed (%d).\n",rc);
      rma2_close(port);
      abort(); 
   }
   rc=rma2_noti_get_block(port, &notip);
   if (rc!=RMA2_SUCCESS)
   {
      fprintf(stderr,"Notification failed (%d).\n",rc);
      rma2_close(port);
      abort(); 
   }
   rma2_noti_free(port,notip);
   \endcode
   In this example, the complete buffer is sent to the remote process, that was selected above using rma2_connect. The destination address is given by
   the dest_address, which is the result of memory registration on the remote node. The generation of both a completer and a requester notification
   is requested. Thus, after posting the operation, the process waits for a new notification, which signals the completion of an operation.
   
   There are a number of different commuication command available in RMA2:
   - Put
   - Get
   - Notification Put
   - Immediate Put
   - Locks
   
   Put operations transfer data from the requester (or initiator) to a target (write into remote memory). Get requests work the other way around 
   (read from remote memory). Both, get and put requests are available as byte and quadword aligned versions. 
   Advice: Use the quadword aligned version wherever possible, it may perform better.
   If the quadword aligned command are used, both operands must reside on quadword (64 bit) aligned addresses!
   
   The immediate put operations takes a 64 bit value and writes it directly to the specified remote memory location. It is thus a special form
   of a sized put operation, with a size >= 8 bytes and the payload being transfered to the device in the descriptor (and not via DMA).
   
   The notification put operations,only triggers notifications. A payload of 64 bit and an additional payload of 8 bit (class) may be specified.
   Notification puts can be used to exchange protocol information or sychronize endpoints.
   
   Lock operations can be used for more elaborate sychronization operations. A description follows later on.
   
   \subsection NOT Notifications
   Notifications are an important part of the RMA2 API. Therefore handling of notifications is described here in more detail to give the
   reader a better understanding of how the notification system of RMA2 works.
   #FIXME: TBA
   
   \subsection LOCK Locks
   #FIXME: TBA
   
   \subsection Fin Finalization
   Now, the remaining part is to clean-up:
   \code
   rma2_unregister(port,send_region);
   rma2_disconnect(port,handle);
   rma2_close(port);
   \endcode

   This concludes this small introduction to the usage of the librma2 API.

   \section MOD Modules: 
   - \ref TYPES Reference of RMA2 types
   - \ref FUNC Reference of RMA2 Functions
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdint.h>
#include <sys/uio.h>
#include <stdio.h>

//The data type RMA2_Region is shared with kernel space an lives in its own header file, which is included here.
#include "extoll2_list.h"
#include <rma2_region.h>


/*!
  \defgroup TYPES Datatypes of LIBRMA2

  @{
*/

/*!
  \brief  Return value of all LIBRMA2 functions
   
  All LIBRMA2 functions return a value of type RMA2_ERROR. If the function succeeded,
  a value of RMA2_SUCCESS (=0) is returned. The functions rma2_perror and 
  rma2_serror may be used to print error messages in human readable format.
*/

//FIXME: why are some values positive? possible to change without API break? is the description a full sentence? start with upper or lower case?
  typedef enum {RMA2_SUCCESS = 0, //!< return of all successful finished RMA2 functions
		RMA2_ERR_ERROR = -1, //!< severe error or unkown error, should never occur
		RMA2_ERR_INV_HANDLE =-2, //!< Invalid virtual connection handle
		RMA2_ERR_INV_PORT=-3, //!< Invalid Port.
		RMA2_ERR_INV_VALUE=-4, //!< invalid parameter value
		RMA2_ERR_NO_ROUTE=-5, //!< No route can be found!
		RMA2_ERR_PORTS_USED=-6, //!< All ports used.
		RMA2_ERR_NO_MEM=-7,//!< Memory can not be registered 
		RMA2_NO_NOTI=-9, //!< There was no notification.
		RMA2_ERR_POLL_EXCEEDED=-10, //!< poll limit exceeded.
		RMA2_ERR_WRONG_ARG=-11, //!< Wrong argument in an rma2 function call
		RMA2_ERR_NOT_YET=-12, //!< requested feature is not yet implemented.
		RMA2_ERR_NO_DESC_MEM=-13, //!< no free descriptor space available. (Try again later)
		RMA2_ERR_NO_MATCH=-15, //!< There was no match for the notification
		RMA2_ERR_WRONG_CMD=-16, //!< Wrong command 
		RMA2_ERR_NO_DEVICE=-17, //!< No extoll device found.
		RMA2_ERR_NO_CONNECT=-18, //!< No connection between these ports exists.
		RMA2_ERR_INVALID_VERSION=-22, //!< Version of driver and API library do not match
		RMA2_ERR_IOCTL=-23,          //!< Ioctl to RMA2 device driver failed
		RMA2_ERR_MMAP=-24,            //!< mmap of requester address space failed
		RMA2_ERR_FD=-25,   //!< opening of the RMA2 device special file failed

		RMA2_ERR_MAP=-26, //!< memory allocation or free for Region map or table failed
		RMA2_ERR_DOUBLE_MAP=27, //!< try to allocate a 2nd level Region table although one is already present
		RMA2_ERR_NO_TABLE=28, //!< try to access or free a Region table or Region which is not present
		RMA2_ERR_NOT_FREE=29, //!< try to free a ressource which is still in use
		RMA2_ERR_PART=30, //!< tried to use a region with a partial match, where only a full match is allowed
		RMA2_ERR_ATU=31, //!< Error related to an ATU operation occured
		RMA2_ERR_MLOCKLIMIT=32 //!< Error means, that the rlimit MLOCK is reached. Try to unregister regions before continuation.
  } RMA2_ERROR;

  /*!
    \brief Node identifier
    
    Every node in the network has a unique ID. This ID is used to specifiy targets of connection
    requests. One's own node id can be queried using the rma2_get_node_id() function.
    
    Values 0xffff and 0xfffe are reserved!

  */
  typedef uint16_t RMA2_Nodeid;

  /*!
    \brief Virtual process identifier
    
    Every EXTOLL NIC supports a number of virtual endpoints. Each endpoint can be used by a process
    to communicate with the network. An implementation may for example support 256 endpoints.
    Each virtual endpoint has a Virtual Process Identifier, short VPID. This is the type for the VPID.

    Note: Values 0xffff and 0xfffe are reserved.
  */
  typedef uint16_t RMA2_VPID;

  /*!
    \brief A Network logical address represents registered memory for the RMA2 unit

    The RMA2 unit uses services from the EXTOLL Address Translation Unit (ATU) to access
    main memory and perform a logical to physical address translation. Addresses to the
    RMA2 unit have to be given in terms of Network Logical Addresses, short NLAs. This
    type represenst a RMA2 NLA. A NLA can be obtained for a registered memory region
    using the \ref rma2_get_nla function.
  */
  typedef uint64_t RMA2_NLA;

  /*!
  \brief Type to specify the notifications a command can cause.

  In EXTOLL RMA2, a command can cause three types of notifications:
  \li requester notifications
  \li completer notifications
  \li responder notifications
  Using a RMA2_notification_spec it is possible to request that a command
  causes a certain notification or not. 
  A command may only cause notifications at units it actually passes through:
  \li all put-like commands may cause requester and completer notifications
  \li all get-like comands (including locks) may cause all notifications
  There exists a number of predefined constants for the most often used 
  combinations. If a combinations that has no predefined encoding is wanted,
  the different specs may be ored together.
  The RMA2_notification_spec modifies a (basic) command in a number of ways
  and as such increases the flexibility of the RMA2 command system
  considerably.
  Note: If the ERA command modifier for a command is set, a single software transaction may spawn many commands, one for each generated fragment.
  */
  typedef enum {
     RMA2_REQUESTER_NOTIFICATION  = 1, //!< A requester notification should be issued for this command
     RMA2_RESPONDER_NOTIFICATION  = 2, //!< A responder notification should be issued for this command
     RMA2_COMPLETER_NOTIFICATION  = 4, //!< A completer notification should be issued for this command
     RMA2_REQCOMPL_NOTIFICATION   = 5, //!< Both a requester and a completer notification should be issued for this command
     RMA2_RESPCOMPL_NOTIFICATION  = 6, //!< Both a responder and a completer notification should be issued for this command
     RMA2_ALL_NOTIFICATIONS       = 7, //!< All possible notification should be issued for this command
     RMA2_NO_NOTIFICATION         = 0  //!< No notification should be issued for this command
  } RMA2_Notification_Spec;

  /*!
   \brief Type to specify different options to the connect call
   
   
   */
  //FIXME: add member documentation
 typedef enum {
   RMA2_CONN_DEFAULT = 16,
   RMA2_CONN_TC0     = 0,
   RMA2_CONN_TC1     = 1,
   RMA2_CONN_TC2     = 2,
   RMA2_CONN_TC3     = 3,   
   RMA2_CONN_RRA     = 4,
   RMA2_CONN_IRQ     = 8,
   RMA2_CONN_TE      = 16,
   RMA2_CONN_PHYSICAL = 0
 } RMA2_Connection_Options;
 

  /*!
    \brief Encoding of the RMA2 commands

    This is the encoding of all of the commands of the RMA2 unit. In addition, 
    wildcard encodings for the matching functions are given.
    RMA2 Commands are actually 4bit values.
  */
  typedef enum {
    RMA2_BT_PUT          = 2, //!< Byte put command
    RMA2_QW_PUT          = 3, //!< Quadword put command
    RMA2_NOTIFICATION_PUT= 5, //!< Notification Put command (one quadword plus one byte class)
    RMA2_IMMEDIATE_PUT   = 6, //!< Immediate Put command (one byte to one quadword)    

    RMA2_BT_GET          = 0, //!< Byte get command
    RMA2_QW_GET          = 1, //!< Quadword get command
    RMA2_GET_BT_RSP      = 10, //!< Byte get commandresponse command (generated by HW as response to an RMA2_GET_BT)
    RMA2_GET_QW_RSP      = 11, //!< Quadword get commandresponse command (generated by HW as response to an RMA2_GET_QW)
      
    RMA2_LOCK_REQ        = 12,    //!< Lock command (FCAA), request command
    RMA2_LOCK_RSP        = 13,    //!< Lock command (FCAA), response command (generated by HW as response to an RMA2_LOCK_REQ)  
    
    RMA2_PUTS = 7, //!< short for any of the put commands, used for matching notification handling, FIXME
    RMA2_GETS = 14, //!< short for any of the get commands, used for matching notification handling, FIXME
    RMA2_ANY  = 15, //!< short for any  command, used for matching notification handling, FIXME
  }RMA2_Command;
  
  /*!
    \brief Encoding of the RMA2 Command Modifiers

    This is the encoding of all of the commands modifiers of the RMA2 unit. Several of these modifiers may be or'ed together.
    RMA2 Command modifier are actually 4bit values, as they are passed to the requester.
  */
  typedef enum {
    RMA2_CMD_DEFAULT=0, //!< default modifier, i.e. normal transfer without any special features turned on
    RMA2_CMD_MC = 1, //!< this command should be send as a multicast. Destination node id will be interpreted as a multicast group. Multicast must be enabled for this VPID.
    RMA2_CMD_NTR =2, //!< notification replicate modifier. Software requests larger than the MTU will be split. If this bit is set, each fragment generates a notification. Usefull when running on adaptive routing channels
    RMA2_CMD_ERA =4, //!< Excellerate Read Access. RMA2 will not read from main memory but from Excellerate Unit. Advanced usage. Must be enabled for this VPID.
    RMA2_CMD_EWA =8  //!< Excellerate Write Access. RMA2 will not write to main memory but to Excellerate Unit. Advanced usage. Must be enabled for this VPID.
   }  RMA2_Command_Modifier;
  
 /*!
    \brief Encoding of the RMA2 Notification Modifiers

    This is the encoding of all of the modifiers within a RMA2 notification. Several of this modifiers may be or'ed together.
    RMA2 Notification modifiers are actually 6bit values.
  */
  typedef enum {
    RMA2_NOTI_RRA  = 1, //!< command addressed remote registers and not memory
    RMA2_NOTI_IRQ  = 2, //!< command (i.e. notification) caused an interrupt
    RMA2_NOTI_TE   = 4,  //!< command operated on translated addresses or physical addresses
    RMA2_NOTI_EWA  = 8,  //!< Excellerate Write Access. RMA2 will not write to main memory but to Excellerate Unit. Advanced usage. Must be enabled for this VPID.
    RMA2_NOTI_NTR  = 16, //!< notification replicate modifier. Software requests larger than the MTU will be split. If this bit is set, each fragment generates a notification. Usefull when running on adaptive routing channels    
    RMA2_NOTI_ERA  = 32 //!< Excellerate Read Access. RMA2 will not read from main memory but from Excellerate Unit. Advanced usage. Must be enabled for this VPID.
   }  RMA2_Notification_Modifier;
  
 /*!
    \brief Encoding of the RMA2 Notification Error bits

    This is the encoding of all of the error bits within a RMA2 notification. Several of this modifiers may be or'ed together.
    RMA2 Notification errors bits are actually 3bit values.
  */  
   typedef enum {
     RMA2_ERROR_REQ = 1, //!< an error occured in the requester unit
     RMA2_ERROR_RESP =2, //!< an error occured in the responder unit
     RMA2_ERROR_CMPL =4  //!< an error occured in the completer unit
   }  RMA2_Notification_Error;

   /*! 
    \brief Encoding of the RMA2 Replay Buffer options

    This is the encoding of the different possible methods to autmatically drain the replay buffer.
   */
   typedef enum {
     RMA2_REPLAY_MANUAL = 0, //!< do not automatically drain the replay buffer
     RMA2_REPLAY_POST = 1, //!< drain on post descriptor
     RMA2_REPLAY_NOTI = 2, //!< drain while checking for notis
     RMA2_REPLAY_CLOSE = 4, //!< drain when the connection and/or the port is closed
     RMA2_REPLAY_ALL = 7 //!< drain on all events
   } RMA2_Replay_Buffer_Mode;
  /*!
    \brief RMA2_Class is used to differentiate different classes of Notification Put traffic

    RMA2_Class represent the upper 8 bit of the payload of a notification put which
    are encouraged to be used a kind of tag or communication class or handler id
    value. For example an application may choose to allocate class 0 for memory region
    exchange requests, class 1 for memory region answer start, class 2 for memory region
    payload and class 3 for memory region end responses, while other classes remain
    to exchange single double values, perform sychronization etc.
    Note that class 255 (0xff) and 254 (0xfe) should not be used, as these values are reserved for matching
    purposes. A notification put operation takes one argument of this type, it will be transported in the immediate_value_high field (see also RMA2_Notification).
  */
  typedef uint8_t RMA2_Class;

  /*!
    \brief This struct represents a notification entry
    
    This struct represents any of the possible notification entries. Where fields have different meaning
    depending on the notification type, a combination of union and structs has been used.
    An entry in the notification queue is always of this type and can be manipulated using the notification API (\ref NOTI).
  */

//FIXME: add member-description
#pragma pack(1)
  typedef struct {
    union {
      uint64_t value;
    } word0;
    union {
      uint64_t value;
    } word1;
//FIXME: is this still used and correct? should it be updated to the current layour of the notification?  
/*    union {
      uint64_t local_address; //!< NLA or physical address involved on this node, used for put/get commmands
      uint64_t immediate_value; //!< immediate value (for notification put operations)
      struct {
	uint16_t lock_id_low; //!<  bit 0-15 of the lock id
	uint8_t  lock_id_high; //!< bit 16-23 of the lock id
	uint8_t  lock_result; //!< A requester notification from a lock operation contains a 0, otherwise it contains a 1 if successfull, 0 otherwise
	uint32_t lock_value; //!< lock value after the operation was performed (successfully or not)
      } lock; //! < used for lock operation, encodes lock id, result and value
    } operand;
    union {
      struct {
	uint16_t   payload_length_low; //!< bit 0-15 of the payload size
	uint8_t    payload_length_high; //!< bit 16-23 of the payload size
      } length; //! < used for put/get operation, codes the payload length
      struct {
	uint16_t   target; //!< 0 if the target of the lock was the Completer, 1 if the target was the responder
	uint8_t    reserved; //!< reserved
      } target; //! < used for lock operation, contains the target unit
      struct {
	uint16_t   immediate_value_high; //!< additional 9 bit of payload of the notification put are encoded in this field
	uint8_t    reserved; //!< reserved
      } immediate_high; //! < used for immediate put operations, contains additional payload
    } info;
    uint8_t    remote_vpid;  //!< 8 bit VPID field
    uint16_t   command_field; //!< 3 bit notification, 4 bit command, 6 bit mode and 3 bit error fields
    uint16_t   remote_nodeid; //!< the remote node that was involved in this command*/
  } RMA2_Notification;
#pragma pack()
  //internal specification for RMA2...
  #include "sys_rma2.h"

  /*!
    \brief Handle for a (virtual) connection.
    
    Before data transfer functions can be called, a virtual connection must be established between
    two VPIDs, regardless wheter they reside on the same or on different nodes. The RMA2_handle type
    represents such an connection and is an opaque handle for it.
  */
  typedef RMA2_Connection* RMA2_Handle;

  /*!
  \brief  Handle for an RMA2 endpoint.
   
  Each endpoint used in LIBRMA2 is referenced using a variable of
  type RMA2_Port. Almost all functions take an RMA2_Port as
  first parameter.
  \par
  rma2_open allocates a new RMA2_Port and returns a pointer of type
  RMA2_Port*, RMA2_close deallocates the endpoint again.
  \par
  User applications should never use RMA2_Port or the underlying 
  structure directly.
  */
  typedef RMA2_Endpoint*  RMA2_Port;

 /*!
    \brief RMA2_Descriptor represents a RMA2 command descriptor.

    The RMA2_Descriptor type is used internally for the all-in-one command posting functions and
    explicitely using the desriptor API functions (\ref DESC) together with rma2_post_descriptor.
    
    The RMA2_Descriptor struct has been designed to be 64 byte large with the second half being
    the data that has to be transferred to the hardware. If the complete struct is aligned to
    a 16-byte boundary, also the second half is aligned and can be loaded using movdqa into
    an xmm register (SSE2).

	In Hardware, a command descriptor consists of a 192-bit payload and an address part, which
    also encodes information.
	The address part is defined as follows:
	{ TE,INT,RRA, TC,SOURCE_VPID | reserved },
	where the page boundary is signified by the pipe symbol here.
	The node and vpid IDs are set when calling rma2_connect (kernel). The bits below the
	page boundary are set by the descriptor functions of librma2.
	The payload is competely set by the corresponding librma2 functions.
	The following 3 fields are additionally checked against mask registers by the hardware,
	to check if the access is actually allowed. RRA is a one bit field, if set it means that
	the command should perform a remote register file access (not enabled for user processes by default).
	The IRQe bit requests that hardware generates an interrupt for every notification written to
	system memory. Per defualt this is alos not accessible by user soace software.
	The final bit, the TAE bit, requests ATU Translation for main memory addresses. This should always
	be set by user space software. If the corresponding maks bit is not set, hardware forces a translation
	though, independent of this bit. The lowest 3 bits are reserved and should remain zero.
  */
  typedef struct {
    extoll_list_head_t list; //!< field to manage descriptors in the descriptor list
    RMA2_Handle handle; //!< this is the handle this descriptor should be used for
    uint64_t reserved0; //!< reserved field
    uint64_t value[3]; //!< the encoded value of the three words of the descriptor as it is passed to hardware
    uint64_t reserved1; //!< reserved field    
  } RMA2_Descriptor;

  /*!    \brief Wildcard to match for a notification put with any class  */
  extern const RMA2_Class RMA2_CLASS_ANY;
  /*! \brief Wildcard to match against a notification originating from any node */
  extern const RMA2_Nodeid RMA2_NODEID_ANY;
  /*! \brief Wildcard to match against a notification originating from any vpid */
  extern const RMA2_VPID RMA2_VPID_ANY;
  /*! \brief Wildcard to match against a notification from any lock id */
  extern const uint32_t RMA2_LOCK_ANY;
 /*!  \brief This constant can be passed to the match function to match on any NLA. */
  extern const RMA2_NLA RMA2_ANY_NLA;
  
  /*! \brief Flag for notification matching: just perform a default notification match, no special operation requested*/
  extern const uint32_t RMA2_DEFAULT_FLAG;
  /*! \brief Flag for notification matching: perform a blocking match operation, i.e. do not return before a match could be completed */
  extern const uint32_t RMA2_BLOCK_FLAG;
  /*! \brief FLAG for notification matching: NLA to match is the last of NLA of the transfer, not the first. Usefull for MPI implementations. */
  extern const uint32_t RMA2_MATCH_LAST_NLA_FLAG;
  /*!
  @}
*/

/*!
  \defgroup FUNC RMA2 Functions

  @{ 
  The following groups of functions are defined by the LIB RMA2 library:

  - \ref OPENCLOSE Open/close/connect (slow functions)
  - \ref CMD       Issuing of different comands
  - \ref DESC      Assembling and managing command descriptors
  - \ref NOTI      Handling, polling etc of notifications
  - \ref MEM       Memory region management
  - \ref UTIL      Utility functions.

  Slow functions means, that the function may  block in system calls. 

*/

/*!
  \defgroup OPENCLOSE Open/close/connect functions

  @{
*/
/*! 
  \brief Opens an RMA2 endpoint

  \b Note: This function must be called before any other LIBRMA2 function!

  This function searches for next available endpoint (if any). It then
  opens the endpoint and initializes it. A RMA2_Port is allocated and
  returned.
  This function also allocates a default sized notification queue. You can use the provided functions to
  query the size of this queue and resize it, if desired. Note that this should be done prior to any traffic.
  Also, every endpoint is associated with a protection key, which is initialized with 0 per default. Use the provided function to change this.

  \param port return parameter for the newly opened endpoint or undefined if not successfull (valid adress
              must be specified)
  \return \li RMA2_SUCCESS               on success    
          \li RMA2_ERR_IOCTL             error while communicating with the Extoll/RMA2/ATU device driver
          \li RMA2_ERR_MMAP              error while trying to mmap dma memory
          \li RMA2_ERROR                 internal error
          \li RMA2_ERR_NO_DEVICE         no Extoll device found
          \li RMA2_ERR_PORTS_USED        all endpoint are in use, and therefor not
                                        available to the calling application
          \li RMA2_ERR_FD                opening of /dev/rma2 failed
          \li RMA2_ERR_INVALID_VERSION   device driver version and API version do not match
    
    
*/
RMA2_ERROR rma2_open(RMA2_Port* port);

/*!
  \brief Closes an RMA2 endpoint.
  
  This function closes the endpoint and frees all memory associated with it. The RMA2_Port
  structure is no longer valid after this function has been called!

  \param port                     port to be closed
  \return \li RMA2_SUCCESS         on success
          \li RMA2_ERR_MMAP        error while unmapping address space
          \li RMA2_ERROR           internal error 
          \li RMA2_ERR_INV_PORT    the specified port_id does not identify
                                  a valid, opened hostport.
    
*/
RMA2_ERROR rma2_close(RMA2_Port port);

/*!
  \brief Establishes a virtual connection to another RMA2 endpoint.

  A virtual connection between the local endpoint and a specified port is
  instantiated. A handle for this connection is returned.
  By default, for this connection Translation is enabled, interrupt generation is disabled, remote register access is disabled and the default traffic class is used.
  As an advanced usage, these bits can be set using the provided functions (See FIXME).

  \param hport       port from which the virtual connection is to be established
  \param dest_node   node id of the remote peer
  \param dest_vpid   VPID of the remote peer
  \param options     different options that can be set for the connection
  \param handle      return parameter for the RMA2_Handle representing this connection (valid adress
                     must be specified)
  
  \return \li RMA2_SUCCESS            on success
          \li RMA2_ERR_INV_PORT       one of the specified port does 
                                     not identify a valid, opened endpoint.
          \li RMA2_ERR_NO_ROUTE       no route could be found from this endpoint to 
                                     the destination endpoint
   
*/   
RMA2_ERROR rma2_connect(RMA2_Port hport,RMA2_Nodeid  dest_node, RMA2_VPID dest_vpid ,RMA2_Connection_Options options, RMA2_Handle* handle);


/*!
  \brief Closes a virtual connection to another endpoint.

  \b Warning: After calling RMA_disconnect you must not issue
  commands to the remote endpoint over this connection anymore.

  \param port   the port from which the virtual connection is to be removed
  \param handle the connection that is to be closed 
   
  \return \li RMA2_SUCCESS         on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
          \li RMA2_ERROR           internal error 
*/
RMA2_ERROR rma2_disconnect(RMA2_Port port, RMA2_Handle handle);


/*!
 \brief Returns the destination node of a virtual connection

 \param handle The connection to be queried
 \return The connection's destination nodeid
*/
RMA2_Nodeid rma2_handle_get_node(RMA2_Handle handle);

/*!
 \brief Returns the destination VPID of a virtual connection

 \param handle The connection to be queried
 \return The connection's destination VPID
*/
RMA2_VPID rma2_handle_get_vpid(RMA2_Handle handle);


/*!
  @}
*/

/*!
  \defgroup CMD       Issuing of different comands

  This group of functions issues commands to the hardware. Generally, EXTOLL RMA2
  is a os-bypass protocoll, i.e. the kernel is not invovled when new commands
  are posted to the NIC. All operand addresses for commands have to specified
  using memory region handles and offsets. Operations that span more than one page,
  must originate and point to memory regions that were registered in a single rma2_register_region call 
  (at least if translation is employed).
  Operations that are longer than one MTU, will be fragmented by hardware. If adaptive transport is selected,
  consider using the ERA modifier.
  There are two flavors of put/get operations: byte and quadword. The only difference is, that in the quadword
  case addresses must be quadword aligned and the length must be a multiple of 8,
  In any case, the maximum size in byte that can be requested with a single operation is 2**23=8MB, regardless
  if this is a byte or quadword put or get.
  @{
*/

/*!
  \brief Posts a byte-put command to the RMA2 unit

  This function posts a byte put comand to the RMA2 unit.
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param src_region   source region, from where the data should be putted (local)
  \param src_offset   offset into the src_region where the hardware should start to read data
  \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address at the destination (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
  RMA2_ERROR rma2_post_put_bt(RMA2_Port port, RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, 
			      RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);
  
  /*!
  \brief Posts a byte-put command to the RMA2 unit without a region

  This function posts a byte put comand to the RMA2 unit. It differs from rma2_post_put_bt that it uses an NLA and not a Region as source address parameter.
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param src          source address (RMA2_NLA), from where the data should be putted (local, (see also \ref rma2_get_nla).
   \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address at the destination (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
  RMA2_ERROR rma2_post_put_bt_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA src, uint32_t size, RMA2_NLA dest_address, 
				     RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);
/*!
  \brief Posts a quadword-put command to the RMA2 unit

  This function posts a quadword put comand to the RMA2 unit. Note that addresses must be quad-word aligned (8 byte) and the size must be
  a multiple of 8 byte.
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
  
  \param port         endpoint used  
  \param handle       handle of the virtual connection to which the command should be posted
  \param src_region   source region, from where the data should be putted (local)
  \param src_offset   offset into the src_region where the hardware should start to read data
  \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address at the destination (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command                      

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_post_put_qw(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, 
			     RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);

  /*!
  \brief Posts a quadword-put command to the RMA2 unit without a region

  This function posts a quadword put comand to the RMA2 unit. It differs from rma2_post_put_qw that it uses an NLA and not a Region as source address parameter.
  Note that addresses must be quad-word aligned (8 byte) and the size must be a multiple of 8 byte.
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param src          source address (RMA2_NLA), from where the data should be putted (local, (see also \ref rma2_get_nla).
   \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address at the destination (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid
	  \li RMA2_ERROR           internal error
*/
RMA2_ERROR rma2_post_put_qw_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA src, uint32_t size, RMA2_NLA dest_address, 
				   RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);
  
/*!
  \brief Posts a byte-get command to the RMA2 unit

  This function posts a byte get comand to the RMA2 unit.
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
 
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param src_region   source region, to where the data should be get (local)
  \param src_offset   offset into the src_region where the hardware should start to write data
  \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address at the remote peer (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for 
  \param modifier     field for additional (advanced) modifiers to the command                      

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_post_get_bt(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, 
			    RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);

 /*!
  \brief Posts a byte-get command to the RMA2 unit without a region

  This function posts a byte get comand to the RMA2 unit. It differs from rma2_post_get_bt that it uses an NLA and not a Region as source address parameter
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param src          source address (RMA2_NLA), to where the data should be get(local, (see also \ref rma2_get_nla).
  \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address  at the remote peer (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command                      

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/
RMA2_ERROR rma2_post_get_bt_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA src,  uint32_t size, RMA2_NLA dest_address, 
				   RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);

/*!
  \brief Posts a cacheline-get command to the RMA2 unit

  This function posts a cacheline get comand to the RMA2 unit.Note that addresses must be quad-word aligned (8 byte) and the size must be
  a multiple of 8 byte.
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param src_region   source region, to where the data should be get (local)
  \param src_offset   offset into the src_region where the hardware should start to write data
  \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address at the remote peer (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command                      

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_post_get_qw(RMA2_Port port,RMA2_Handle handle, RMA2_Region* src_region, uint32_t src_offset, uint32_t size, RMA2_NLA dest_address, 
			    RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);

 /*!
  \brief Posts a cacheline-get command to the RMA2 unit without a region

  This function posts a cacheline get comand to the RMA2 unit. It differs from rma2_post_get_qw that it uses an NLA and not a Region as source address parameter.
  Note that addresses must be quad-word aligned (8 byte) and the size must be a multiple of 8 byte.
  The size of the put is specified in bytes with a maximum of 8,388,608 bytes. Transactions larger than one EXTOLL R2 MTU will
  automatically be fragmented in hardware.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param src          source address (RMA2_NLA), to where the data should be get(local, (see also \ref rma2_get_nla).
  \param size         number of byes that should be put: between 1 and 8,388,608 are legal values.
  \param dest_address The network logical address  at the remote peer (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid
	  \li RMA2_ERROR           internal error
*/
RMA2_ERROR rma2_post_get_qw_direct(RMA2_Port port,RMA2_Handle handle, RMA2_NLA  src, uint32_t size, RMA2_NLA dest_address, RMA2_Notification_Spec spec,
				   RMA2_Command_Modifier modifier);

/*!
  \brief Posts remote (or local) lock command to the RMA2 unit.

  Extoll RMA2 support hardware, atomic locks with a function called FCAA, fetch-compare-and-add.
  This function is accessible through the RMA2 lock command, which is posted by this function.
  Each endpoint has a pool of 16k locks available, address by the lock_number parameter.
  The cmp_value and the add_value are the operands to the FCAA operation. The operation
  is performed at the remote peer. It is usually necessary to open a virtual connection
  to itself to be able to also perform lock operations for a local lock.
  Lock operations are split-phase, this function only posts the lock requests. A Lock notification
  will arrive at a later time informing the called if the lock was successfull and what the 
  updated lock value is.
  Lock operations can be addressed to the responder or completer, to sychronize with the respective path. By addressing a lock to both units,
  read and write operations can be sychronized.
  
  Pseudo-code for the FCAA operation:
  \code
  if (*destination <= compare)
    *destination += add;
    return (*destination, true);
  else
    return (*destination, false);
  \endcode

  \param port         endpoint used  
  \param handle       handle of the virtual connection to which the command should be posted
  \param target       0 to target completer, 1 to target responder unit
  \param lock_number  lock identifier
  \param cmp_value  compare value for the FCAA operation
  \param add_value add operand for the FCAA operation  
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_post_lock(RMA2_Port port,RMA2_Handle handle,uint32_t  target, uint32_t lock_number, int32_t cmp_value, int32_t add_value, 
			  RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);

/*!
  \brief Posts a notification put command to the RMA2 unit

  This function posts a notification put comand to the RMA2 unit. A notification put transfers a 64-bit value
  and a 8-bit tag, called class, to the remote peer and puts these into a completer notification. Notification
  puts are especially usefull, if no remote NLAs are known and can be used to run a bootstrap protocol or for synchronization.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param class        The 8-bit tag or class, which is transmitted
  \param value        64-bit sized payload
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command                      

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_post_notification(RMA2_Port port,RMA2_Handle handle, RMA2_Class mclass, uint64_t value, 
				  RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);

/*!
  \brief Posts a so-called immediate put command to the RMA2 unit

  This function posts an immediate put comand to the RMA2 unit. An immediate put transfers a 64-bit value
  to the specified remote NLA. Notification handling is as with other put operations. This command is 
  usefull if only a single quadword has to be updated at the remote side.
  The byte_mask feature allow also to transfer arbitraty data from length 1 to 8.
  
  \param port         endpoint used
  \param handle       handle of the virtual connection to which the command should be posted
  \param count        how many bytes to write (allowed values are 1 to 8)
  \param value        64-bit immediate value that should be written to remote memory
  \param dest_address The network logical address at the remote peer (see also \ref rma2_get_nla).
  \param spec         Specifies the notification that this command should cause. It is best practice to use the predefined macros
                      provided for this
  \param modifier     field for additional (advanced) modifiers to the command                      

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_post_immediate_put(RMA2_Port port,RMA2_Handle handle, uint32_t count, uint64_t value, RMA2_NLA dest_address, 
				   RMA2_Notification_Spec spec, RMA2_Command_Modifier modifier);

/*!
  \brief Posts a pre-computed command to the RMA2 unit.

  This function posts a descriptor to the RMA2 unit which is pre-computed or prepared
  using the functions from the \ref DESC group.

  \param port         endpoint used
  \param handle       connection handle used
  \param desc Descriptor specifying the RMA2 operation to be performed.
  
  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
  RMA2_ERROR rma2_post_descriptor(RMA2_Port port, RMA2_Handle handle, RMA2_Descriptor* desc);
/*!
  @}
*/

/*!
  \defgroup DESC      Assembling and managing command descriptors

  This group of functions can be used to build descriptors of RMA2 commands. The resulting descriptor can then be passed
  on to the network hardware using the \ref rma2_post_descriptor function. In general, all of the functionality
  can also be used by the short-form functions described in the Command section. Actually, these functions are implemented
  in terms of the descriptor functions and the rma2_post_descriptor. The functions can for example be used, to 
  optimize repetetive tasks somewhat.
  One important restrictions in relation to descriptors is, that they need to be 16-byte aligned. The \ref rma2_desc_alloc
  function takes care of this restriction. If descriptors are allocted in another way the following points may help you:
  - if using the heap, consider using posix_memalign:
  \code
  #define _XOPEN_SOURCE 600
  #include <stdint.h>
  [...]
  RMA2_Descriptor desc;
  result=posix_memalign(&desc, 16,sizeof(desc));
  if (result!=0)
   { ...handle error... }
   \endcode
  - is using automatic or global variables, try using 
  \code
  RMA2_Descriptor desc __attribute__ ((aligned (16)));
  \endcode

  Some examples should help to understand how to use these functions:
  
  tba.

  @{
*/
/*!
  \brief Allocate an Descriptor
  
  This function returns an allocated descriptor. It can be used to quickly allocate a descriptor that satisfies
  all restrictions. Internally, the descriptor is allocated from a pre-malloced descriptor table, so no
  system call can be executed because of this function, except when the pre-malloced list is empty. In this case a 
  new batch of descriptor will be allocated from the system using malloc.
  An descriptor using this method should be returned using \ref rma2_desc_free. There is a limited amount of
  descriptor available, see \ref rma2_desc_query_free and \ref rma2_desc_query_avail.
  
  \param port the endpoint the descriptor is allocated from which it will be used on
  \param desc A pointer to a pointer variable that will hold a reference to the newly created desc.

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_NO_DESC_MEM there are no free descriptors available, consider freeing some using rma2_desc_free
	  \li RMA2_ERROR           internal error 
*/
  RMA2_ERROR rma2_desc_alloc(RMA2_Port port, RMA2_Descriptor** desc);

/*!
  \brief Frees a descriptor again.
  
  This function frees a descriptor previously allocate using rma2_desc_alloc and returns it to the pool of available descs.

  \param port The endpoint the descriptor was allocated from
  \param desc A pointer to a pointer variable that will be freed

  \return \li RMA2_SUCCESS    on success
	  \li RMA2_ERROR           internal error 
*/
RMA2_ERROR rma2_desc_free(RMA2_Port port, RMA2_Descriptor* desc);

/*!
  \brief Returns the number of currently free descriptors for use with rma2_desc_alloc.

  This number of descriptors can be used, without a call to malloc. If more descriptors are needed, librma2 will allocate memory
  for more descriptors, but this will require a call to malloc.
  \param port endpoint to be queried
  \param num pointer to an integer holding the number of free desc's upon return
  \return \li RMA2_SUCCESS
*/
RMA2_ERROR rma2_desc_query_free(RMA2_Port port, int* num);

//FIXME: not documented
RMA2_ERROR rma2_desc_set_put_get(RMA2_Descriptor* desc, RMA2_Handle handle,RMA2_Command command, uint32_t count, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, uint64_t read_address, uint64_t write_address);
//FIXME: not documented
RMA2_ERROR rma2_desc_set_immediate_put(RMA2_Descriptor* desc, RMA2_Handle handle, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, uint32_t count, uint64_t write_address, uint64_t value);
//FIXME: not documented
RMA2_ERROR rma2_desc_set_noti_put(RMA2_Descriptor* desc, RMA2_Handle handle, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, RMA2_Class mclass, uint64_t value);
//FIXME: not documented
RMA2_ERROR rma2_desc_set_lock(RMA2_Descriptor* desc, RMA2_Handle handle, RMA2_Notification_Spec noti,
			  RMA2_Command_Modifier modifier, uint8_t type, uint32_t lock_number, uint32_t compare, uint32_t add_value);

//FIXME: not used anymore?
/*!
  \brief Sets the destination, command, and size
  
  This function sets the destination, the command and the size argument in the descriptor.
  The destination is given in terms of a virtual connection handle (RMA2_Handle).

  \param desc pointer to the descriptor that is manipulated.
  \param handle the destination to be set
  \param command the RMA2 command to be set
  \param count the count argument for the RMA2 command, command specific, see documentation of RMA2_Command
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set(RMA2_Descriptor* desc, RMA2_Handle handle, RMA2_Command command, uint32_t count);

//FIXME: not used anymore?
/*!
  \brief Sets the source and destination memory addresses 
  
  This function sets the destination and source memory addresses in the descriptor.
  These addresses are given as a RMA2_NLA.

  \param desc pointer to the descriptor that is manipulated.
  \param source The source address for the descriptor
  \param destination The destination address for the descriptor
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set_source_destination(RMA2_Descriptor* desc, RMA2_NLA source, RMA2_NLA destination);

//FIXME: not used anymore?
/*!
  \brief Sets the source memory addresses 
  
  This function sets the source memory address in the descriptor.
  The address are given as a RMA2_NLA.

  \param desc pointer to the descriptor that is manipulated.
  \param source The source address for the descriptor
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set_source_address(RMA2_Descriptor* desc, RMA2_NLA source);

//FIXME: not used anymore?
/*!
  \brief Sets the destination memory addresses 
  
  This function sets the destination memory address in the descriptor.
  The address are given as a RMA2_NLA.

  \param desc pointer to the descriptor that is manipulated.
  \param destination The destination address for the descriptor
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set_destination_address(RMA2_Descriptor* desc, RMA2_NLA destination);

//FIXME: not used anymore?
/*!
  \brief Sets the immediate value for the descriptor
  
  This function sets the immediate value for use in a RMA2_IMMEDIATE_PUT  in the descriptor.
  Make sure that you have set the length of the immediate transaction (0-7 for 1 to 8 bytes)
  prior to calling this function.

  \param desc pointer to the descriptor that is manipulated.
  \param value The immediate value to be set
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set_immediate(RMA2_Descriptor* desc, uint64_t value);

/*!
  \brief Sets values for a notification put in the descriptor
  
  This function sets the immediate value for use in a  RMA2_NOTIFICATION_PUT in the descriptor.

  \param desc pointer to the descriptor that is manipulated.
  \param class the class value for the RMA2_NOTIFICATION_PUT
  \param value The immediate value to be set
  \return \li RMA2_SUCCESS
*/
RMA2_ERROR rma2_desc_set_notification_value(RMA2_Descriptor* desc, RMA2_Class mclass, uint64_t value);

//FIXME: not used anymore?
/*!
  \brief Sets the destination in the descriptor
  
  This function sets the destination argument in the descriptor.
  The destination is given in terms of a virtual connection handle (RMA2_Handle).

  \param desc pointer to the descriptor that is manipulated.
  \param handle the destination to be set
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set_destination(RMA2_Descriptor* desc, RMA2_Handle handle);

//FIXME: not used anymore?
/*!
  \brief Sets the command in the descriptor
  
  This function sets the command argument of the descriptor.

  \param desc pointer to the descriptor that is manipulated.
  \param command the command to be set
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set_command(RMA2_Descriptor* desc, RMA2_Command command);

//FIXME: not used anymore?
/*!
  \brief Sets the operands for a remote lock operation in the descriptor
  
  This function sets the operands (add value and compare value) as well as the lock id for
  a remote lock operation (FCAA).
  
  \param desc pointer to the descriptor that is manipulated.
  \param lock_id index or id of the lock to be used on the remote side
  \param cmp_value compare operand for the FCAA operation
  \param add_value add operand for the FCAA operation

  \return \li RMA2_SUCCESS
*/
//  RMA2_ERROR rma2_desc_set_lockop(RMA2_Descriptor* desc, uint32_t lock_id, uint32_t cmp_value, uint32_t add_value);

//FIXME: not used anymore?
/*!
  \brief Sets the notification spec in the descriptor
  
  This function sets the notification spec of the desciptor,
  i.e. which notifications will be generated by this descriptor.

  \param desc pointer to the descriptor that is manipulated.
  \param spec notification spec of the descriptor
  \return \li RMA2_SUCCESS
*/
//RMA2_ERROR rma2_desc_set_notification_spec(RMA2_Descriptor* desc, RMA2_Notification_Spec spec);
/*!
  @}
*/

/*!
  \defgroup NOTI      Handling, polling and matching of notifications

  Notifications are an integral part of the Extoll RMA2 communication system. A command can generate any
  of the following notifications:

  - a requester notification
  - a completer notification
  - a responder notification

  All commands can generate requester and completer notifications. Only get commands and lock commands
  can generate responder notifications. If a notification is requested that is not available for the
  current command, this is silently ignored.

  Thus:

  - a put command can generate a requester notification at the origin node once the command has been completely
  handled (request+paylad send).
  - a put command can generate a completer notification at the target node once the command has been completely
  handled (payload completely written not main memory).
  - a get command can generate a requester notification at the origin node once the command has been completely
  handled (request send).
  - a get command can generate a responder notification at the target node once the command has been completely
  handled (payload completely read from main memory and send to network).
  - a get command can generate a completer notification at the origin node once the command has been completely
  handled (payload completely written not main memory).
  - a lock command works analogous to a get command
  - notification put commands should always specify at least the completer bit, otherwise, the command has no
  effect (other than using bandwidth and network ressources).

  The RMA2 API manages one notification queue per endpoint, which is also the notification queue as seen by the
  hardware. In addition, software transparently manages an extension queue where notifications are moved as soon
  as possible, that is at the end of most API functions (like an MPI progress engine). This helps to clean the
  hardware visible notification queue as soon as possible. It is currenlty a compile-time configuration variable
  if the extension queue feature is used (as well as the size of the queue). The queue ist implemented using a 
  double linked list of pre-allocated entries. This allows for both high performance when inserting new entries
  and when removing entries out of order.

  Blocking notification functions generally poll, i.e. perform a busy blocking. If the kernel version of RMA2 messaging
  is used, it is also possible to use interrupt driven progress; this is reserved for a future revision of the API.

  A match/probe/get does not remove the notification from the queue, i.e. a following call to the same function
  with the same parameters will get the same notification unless it is freed and also, strange things can happen in that
  case. DON'T DO IT. Freeing is made explicit using the rma2_noti_free function. If you need to keep the 
  notification but also want to free it to make ressource available, you probably want to use rma2_noti_dup 
  followed by rma2_noti_free.

  @{
*/
/*!
  \brief Checks (non-blocking) for a new notification.

  This function checks for a new notification. It is non-blocking, if no new notification is
  available, RMA2_NO_NOTI is returned. If a new notification is available, RMA2_SUCCESS is
  returned and a pointer to the notification is stored in the notification parameter.

  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.

  \param port Specifies the enpoint of which the notification queue is checked.
  \param notification pointer to a pointer to a notification. If a notification is found, a pointer to it
  will be stored here, otherwise 0 is stored.  

  \return \li RMA2_SUCCESS            if a new notification is available
          \li RMA2_NO_NOTI            if no new notification is available
          \li RMA2_ERR_INV_PORT       one of the specified port does 
	                               not identify a valid, opened endpoint.
*/
RMA2_ERROR rma2_noti_probe(RMA2_Port port, RMA2_Notification** notification);

/*!
  \brief Waits (blocking) for a new notification.

  This function waits for a new notification. It is a blocking function; if no new notification is
  available, the function waits until one becomes available. If a new notification is available, RMA2_SUCCESS is
  returned and a pointer to the notification is stored in the notification parameter.

  \param port Specifies the enpoint of which the notification queue is checked.
  \param notification pointer to a pointer to a notification. If a notification is found, a pointer to it
  will be stored here, otherwise 0 is stored.
  
  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.

  \return \li RMA2_SUCCESS            if a new notification is available
          \li RMA2_ERR_INV_PORT       one of the specified port does 
	                               not identify a valid, opened endpoint.
*/
RMA2_ERROR rma2_noti_get_block(RMA2_Port port, RMA2_Notification** notification);

/*!
  \brief Matches (non-blocking) for a notification.

  This function checks for a new notification which also matches the given criteria. 
  It is non-blocking, if no new, matching notification is available, RMA2_NO_NOTI is returned. 
  If a new notification is available, RMA2_SUCCESS is  returned and a pointer to the notification 
  is stored in the notification parameter.

  Matching can be performed on the command, the origin/destination nodeid (depending on notification type),
  or origin/destination vpid (depending on notification type). It is also possible to match only for
  requester, completer or responder notifications. See also the \ref NOTI for a more detailed descriptions
  of notification matching.

  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.

  \param port Specifies the enpoint of which the notification queue is checked.
  \param cmd The command that should be matched on (RMA2_ANY for wildcard)
  \param nodeid The nodeid that should be matched on (RMA2_NODEID_ANY for wildcard)
  \param vpid The VPID that should be matched (RMA2_VPID_ANY for wildcard) 
  \param type The type of notification specified in a RMA2_notification spec (RMA2_all_notification for wildcard)
  \param nla NLA to match on, or RMA2_ANY_NLA to wildcard this match parameter
  \param flags additional flags, different value or'ed together. Valid values are RMA2_MATCH_LAST_NLA and  RMA2_BLOCK. See their respective documentation
  \param notification pointer to a pointer to a notification. If a notification is found, a pointer to it
  will be stored here, otherwise 0 is stored.
   
  \return \li RMA2_SUCCESS            if a new notification is available
          \li RMA2_NO_NOTI            if no new notification is available
          \li RMA2_ERR_INV_PORT       one of the specified port does 
	                               not identify a valid, opened endpoint.
*/
RMA2_ERROR rma2_noti_match(RMA2_Port port, RMA2_Command cmd, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification_Spec type, RMA2_NLA nla, int flags, RMA2_Notification** notification);

/*!
  \brief Matches (blocking) for a notification.

  This function waits for a new notification which also matches the given criteria. 
  The function is blocking and only returns if a new, matching notification is available.
  If a new, matching notification is available, RMA2_SUCCESS is  returned and a pointer to the notification 
  is stored in the notification parameter.

  Matching can be performed on the command, the origin/destination nodeid (depending on notification type),
  or origin/destination vpid (depending on notification type). It is also possible to match only for
  requester, completer or responder notifications. See also the \ref NOTI for a more detailed descriptions
  of notification matching.

  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.

  \param port Specifies the enpoint of which the notification queue is checked.
  \param cmd The command that should be matched on (RMA2_ANY for wildcard)
  \param nodeid The nodeid that should be matched on (RMA2_NODEID_ANY for wildcard)
  \param vpid The VPID that should be matched (RMA2_VPID_ANY for wildcard) 
  \param type The type of notification specified in a RMA2_notification spec (RMA2_all_notification for wildcard)
  \param nla NLA to match on, or RMA2_ANY_NLA to wildcard this match parameter
  \param flags additional flags, different value or'ed together. Valid values are RMA2_MATCH_LAST_NLA and  RMA2_BLOCK. See their respective documentation. RMA2_BLOCK is always
  implicitely set for rma2_noti_match_block().
  \param notification pointer to a pointer to a notification. If a notification is found, a pointer to it
  will be stored here, otherwise 0 is stored.
  
  \return \li RMA2_SUCCESS            if a new notification is available
          \li RMA2_NO_NOTI            if no new notification is available
          \li RMA2_ERR_INV_PORT       one of the specified port does 
	                               not identify a valid, opened endpoint.
*/
  RMA2_ERROR rma2_noti_match_block(RMA2_Port port, RMA2_Command cmd, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification_Spec type, RMA2_NLA nla, int flags, RMA2_Notification** notification);


/*!
  \brief Matching check for an Notification Put Completer notification

  This function perfors a matching notification probe for a completer notification containing 
  an notification put completion. Matching is possible in terms of source of the operations (node and
  vpid) and of the class of the operation.
  This function and its blocking peer (\ref rma2_noti_noti_match_block) can be used to easily implement sequential protocols using
  the immediate put facility of EXTOLL RMA2.
  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.
  
  \param port         Endpoint on which to check for the notification.
  \param class        The class that should be matched to, RMA2_CLASS_ANY if all classes should match
  \param nodeid       The source nodeid that should be matched to. RMA2_NODE_ANY if all nodes should match
  \param vpid         The source vpid that should be matched to. RMA2_VPID_ANY if all nodes should match
  \param notification Out-param, points to the notifciation if call was successfull. If the notification
                      is not used by called anymore, \ref rma2_noti_free should be called.

  \param port       handle of the virtual connection to which the command should be posted
  \param class        The class that should be matched to, RMA2_CLASS_ANY if all classes should match
  \param nodeid       The source nodeid that should be matched to. RMA2_NODE_ANY if all nodes should match
  \param vpid         The source vpid that should be matched to. RMA2_VPID_ANY if all nodes should match
  \param notification Out-param, points to the notifciation if call was successfull. If the notification
                      is not used by called anymore, \ref rma2_noti_free should be called.

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_noti_noti_match(RMA2_Port port, RMA2_Class mclass, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification);

/*!
  \brief Performa2 a blocking, matching wait for an Notification Put Completer notification

  This function perfors a matching notification wait for a completer notification containing 
  a notification put completion. Matching is possible in terms of source of the operations (node and
  vpid) and of the class of the operation.
  This function and its non-blocking peer (\ref rma2_noti_noti_match) can be used to easily implement sequential protocols using
  the immediate put facility of EXTOLL RMA2.
  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.
  
  \param port         Endpoint on which to check for the notification.
  \param class        The class that should be matched to, RMA2_CLASS_ANY if all classes should match
  \param nodeid       The source nodeid that should be matched to. RMA2_NODE_ANY if all nodes should match
  \param vpid         The source vpid that should be matched to. RMA2_VPID_ANY if all nodes should match
  \param notification Out-param, points to the notifciation if call was successfull. If the notification
                      is not used by called anymore, \ref rma2_noti_free should be called.

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_noti_noti_match_block(RMA2_Port port, RMA2_Class mclass, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification);

/*!
  \brief Matching check for a Lock Responder notification

  This function perfors a matching notification probe for a responder notification containing 
  an lock completion. Matching is possible in terms of the lock id, destination node and destination vpid of the operation.
  This function and its blocking peer (\ref rma2_noti_lock_match_block) can be used to easily implement sequential locking
  protocols using EXTOLL RMA2.
  
  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.
  
  \param port         Endpoint on which to check for the notification.
  \param lockid       The lock id for which an completion is expected.
  \param nodeid       The source nodeid that should be matched to. RMA2_NODE_ANY if all nodes should match
  \param vpid         The source vpid that should be matched to. RMA2_VPID_ANY if all nodes should match
  \param notification Out-param, points to the notifciation if call was successfull. If the notification
                      is not used by called anymore, \ref rma2_noti_free should be called.

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_noti_lock_match(RMA2_Port port, uint32_t lockid, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification);

/*!
  \brief Matching check for a Lock Responder notification

  This function perfors a matching notification wait for a responder notification containing 
  an lock completion. Matching is possible in terms of the lock id, destination node and destination vpid of the operation.
  This function and its non-blocking peer (\ref rma2_noti_lock_match) can be used to easily implement sequential locking
  protocols using EXTOLL RMA2.
  
  Again, this function only returns a pointer to the notification within the *internal* tables,
  it is absolutely necessary to call \ref rma2_noti_free as soon as possible. If this is a problem,
  use \ref rma2_noti_dup.
  
  \param port         Endpoint on which to check for the notification.
  \param lockid       The lock id for which an completion is expected.
  \param nodeid       The source nodeid that should be matched to. RMA2_NODE_ANY if all nodes should match
  \param vpid         The source vpid that should be matched to. RMA2_VPID_ANY if all nodes should match
  \param notification Out-param, points to the notifciation if call was successfull. If the notification
                      is not used by called anymore, \ref rma2_noti_free should be called.

  \return \li RMA2_SUCCESS    on success
          \li RMA2_ERR_INV_HANDLE  handle is invalid    
	  \li RMA2_ERROR           internal error 
*/         
RMA2_ERROR rma2_noti_lock_match_block(RMA2_Port port, uint32_t lockid, RMA2_Nodeid nodeid, RMA2_VPID vpid, RMA2_Notification** notification);

/*!
  \brief This function copies a notification to an externally supplied location

  The function copies the input notification to the memory pointer to by the output
  parameter. The memory must be allocated by the called. The function is usefull, if
  the notification is going to be used for a longer time (i.e. longer than immediately)
  because notification queue space is a scarce ressource and should be freed as soon as
  possible. This is the reason, why rma2_noti_dup does implicitely free the input
  notification.
  It is possible to use the standard Notification decode functions on the copied 
  notification; it will not be considered for matching anymore of course.
  
  \param input Source notification, notification is automatically freed
  \param output Destination pointer, memory is called allocated

  \return \li RMA2_SUCCESS on success
          \li RMA2_ERROR  internal error
*/
RMA2_ERROR rma2_noti_dup(RMA2_Notification* input, RMA2_Notification* output);

/*!
  \brief Prints a notification to stdout in human readable format

  \param noti The notification to be printed
*/
void rma2_noti_dump(RMA2_Notification* noti);

/*!
  \brief Prints a notification to the given filedesdiptor in human readable format

  \param noti The notification to be printed
  \param filedescriptor The filedescriptor where the notification wil be printed to
*/
void rma2_noti_fdump(RMA2_Notification* noti, FILE * filedescriptor);

/*!
  \brief Prints a notification to the given string in human readable forma2t

  \param noti The notification to be printed
  \param pstring pointer to the C-String, where the notification will be printed to.
  \param size size of pstring in bytes, to avoid buffer-overflows
*/
void rma2_noti_sdump(RMA2_Notification* noti, char* pstring, int size);

/*!
  \brief Decodes the command from the provided notification.
  
  This function decodes the command that caused the provided notification.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The RMA2_Command of the operation that caused the notification to be issued.
*/
RMA2_Command rma2_noti_get_cmd(RMA2_Notification* noti);


/*!
  \brief Decodes the notification type from the provided notification.
  
  This function decodes the notiifcation type, i.e. wehter it is a requester, completer or responder noticiation.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The notiifcation type of the input parameter
*/
RMA2_Notification_Spec rma2_noti_get_notification_type(RMA2_Notification* noti);

/*!
  \brief Decodes the notification mode from the provided notification.
  
  This function decodes the notifcation mode, i.e. wether it access remote register space. causes interupts etc.
  See also RMA2_Notification_Modifier
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The notiifcation mode of the input parameter
*/

RMA2_Notification_Modifier rma2_noti_get_mode(RMA2_Notification *noti);

/*!
  \brief Decodes the error bit-field rom the provided notification.
  
  This function returns the error bits from the nbotification. It is especially usefull to test, if an operation has been
  scucessfully completed, i.e. 
	\code
	if (rma2_noti_get_error(noti)!=0) { 
		... handle error...
	};
  \endcode
  \param noti Pointer to the notification that should be interpreted.
  
  \return Value of the error field
*/
uint8_t rma2_noti_get_error(RMA2_Notification* noti);

/*!
  \brief Decodes the remote node id field of the notification.
  
  This function returns the id of the remote node involved with this operation. If this is a requester notification,
  it is the id of the "destination" or target node, if it is a responder notification it is the source node's id.
  In the completer case, it is either the source node (put style request) or the target node (get style request).
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The id of the remote node
*/
RMA2_Nodeid rma2_noti_get_remote_nodeid(RMA2_Notification* noti);

/*!
  \brief Decodes the remote vpid id field of the notification.
  
  This function returns the vpid of the remote endpoint involved with this operation. If this is a requester notification,
  it is the vpid of the "destination" or target endpoint, if it is a responder notification it is the source endpoint's vpid.
  In the completer case, it is either the source endpoint's (put style request) or the target endpoint's (get style request) vpid.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The id of the remote node
*/
RMA2_VPID rma2_noti_get_remote_vpid(RMA2_Notification* noti);

/*!
  \brief Decodes the local address field of the notification
  
  This function returns the value of the local address field, found in notifications of sized put and get commands.
  The address may either be a NLA (translation enabled) or a physical address (translation enabled). It may also be
  a register or accelerator address, if the respective mode bits are set.
  The local address may be usefull for matching purposes.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The local address
*/
uint64_t rma2_noti_get_local_address(RMA2_Notification* noti);

/*!
  \brief Decodes the payload size field of the notification
  
  This function returns the value of the payload size field, found in notifications of sized put and get commands.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The payload size
*/
uint32_t rma2_noti_get_size(RMA2_Notification* noti);

/*!
 * \brief Calculates the total size of an RMA command.
 *
 * Large RMA comamnds (with payloads bigger than the usable MTU, most sized PUTs and GETs) are splittet into two or more network packages. A completer notification is issued, when the last network package arrives. The local adress in the completer notification is the NLA of the last written payload of the command. The size is the size of the payload in the last network package. So the total size written to the memory must be calculated from those values. The needed additional infomration is the NLA where the command should have started writing to the memory.
 *  
 *  \param start_nla The first NLA where the data is written.
 *  \param noti Pointer to the completer notification.
 *
 *  \return The total size of data written to the memory.
 */
uint32_t rma2_noti_get_total_size(RMA2_NLA start_nla, RMA2_Notification *noti);

/*!
  \brief Returns the 32 bit value of a lock request
  
  This functions is only allowed on notifications for lock commands.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The lock value
*/
uint32_t rma2_noti_get_lock_value(RMA2_Notification* noti);

/*!
  \brief Returns the 24 bit lock number of a lock request
  
  This functions is only allowed on notifications for lock commands.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The lock number
*/
uint32_t rma2_noti_get_lock_number(RMA2_Notification* noti);

/*!
  \brief Returns the result of lock request, i.e. if it was successfull
  
  This functions is only allowed on notifications for lock commands.
  Returns a 1 if the lock command was successfull, otherwise a zero.
  Requester notifications from locks always return a zero, since no comparison has taken place yet.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The result of the lock
*/
uint8_t rma2_noti_get_lock_result(RMA2_Notification* noti);

/*!
  \brief Returns the 64 bit payload of a notification put
  
  This functions is only allowed on notifications for notification put commands.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The notification put payload
*/
uint64_t rma2_noti_get_notiput_payload(RMA2_Notification* noti);

/*!
  \brief Returns the 8 bit class of a notification put
  
  This functions is only allowed on notifications for notification put commands.
  
  \param noti Pointer to the notification that should be interpreted.
  
  \return The notification put class
*/
uint8_t rma2_noti_get_notiput_class(RMA2_Notification* noti);


 /*!
    \brief Free a notification again.
    
    Notification queue space is a scarce ressource. The differen functions returning
    a notification do not remove the notification from its queue; it is thus necessary to
    "free" the notification or mark it as cosumed. This is performed by rma2_noti_free.

    It is paramount to call this function as soon as possible and recomended to do so
    immediately after havin found the notification. If the content has to be preserved,
    consider using \ref rma2_noti_dup.

    \param port The RMA2 endpoint
    \param notification pointer to the notification to be freed

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_INV_VALUE the given descriptor is not located within the notification queue
            \li RMA2_ERROR internal error
 */
RMA2_ERROR rma2_noti_free(RMA2_Port port, RMA2_Notification* notification);

/*!
  @}
*/

/*!
  \defgroup MEM       Memory region management

  @{
*/
/*!
    \brief Register the given memory region

    This functions registers the given memory region with the EXTOLL NIC. After
    succesfull registration, the memory can be used as source and destination for
    RMA2 commands. This function may be slow, i.e. perform a system call.
    The returned pointer must not be changed by the calling program. The unchanged pointer/structure
    must be used to unregister the region when it is no longer needed.

    \param port The RMA2 endpoint for which memory is registered
    \param address Start address of the memory area to be registered
    \param size Number of bytes to be registered. This value is rounded to the next
                higher multiple of 4k, since only pages can be registered.
    \param region Output parameter. This called allocated struct is filled with the
                actual result of the memory registration.

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_NO_MEM memory could not be registered
            \li RMA2_ERR_INV_PORT port was invalid
*/
RMA2_ERROR rma2_register(RMA2_Port port, void* address, size_t size, RMA2_Region** region);
RMA2_ERROR rma2_register_nomalloc(RMA2_Port port, void* address, size_t size, RMA2_Region* region);
RMA2_ERROR rma2_register_cached(RMA2_Port port, void* address, size_t size, RMA2_Region** region);

/*!
    \brief Un-register the given memory region

    This functions un-registers the given memory region with the EXTOLL NIC. After
    succesfull de-registration, the memory can no longer be  used as source and destination for
    RMA2 commands. This function may be slow, i.e. perform a system call.

    \param port The RMA2 endpoint for which memory is un-registered   
    \param region The region to be un-registered

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_INV_PORT port was invalid
*/
RMA2_ERROR rma2_unregister(RMA2_Port port, RMA2_Region* region);
RMA2_ERROR rma2_unregister_nofree(RMA2_Port port, RMA2_Region* region);
RMA2_ERROR rma2_unregister_cached(RMA2_Port port, RMA2_Region* region);
/*!
    \brief Register the given adhoc memory region

    This functions registers the given memory region with the EXTOLL NIC for adhoc usage.
    This means, it may be used for local access only and the size is also fixed to one page.

    Applications should always choose register memory using this function, if the source of an
    RMA2 request may not be residing in a registered region.

    This function is able to handle overlapping registration with regions that have been
    registered using rma2_register without problems, while overlapping regitstration using
    rma2_register is not allowed.

    \param port The RMA2 endpoint for which memory is registered
    \param address Start address of the memory area to be registered
    \param region Output parameter. This called allocated struct is filled with the
                actual result of the memory registration.

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_NO_MEM memory could not be registered
            \li RMA2_ERR_INV_PORT port was invalid
*/
RMA2_ERROR rma2_register_adhoc(RMA2_Port port,void* address, RMA2_Region** region);

/*!
    \brief Un-register the given adhoc memory region

    This functions un-registers the given adhoc memory region with the EXTOLL NIC. 

    After succesfull de-registration, the memory can no longer be  used as source and destination for
    RMA2 commands. This function may be slow, i.e. perform a system call.

    Note that the function also employs a registration cache functionality.

    \param port The RMA2 endpoint for which memory is un-registered   
    \param region The region to be un-registered

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_INV_PORT port was invalid
*/
RMA2_ERROR rma2_unregister_adhoc(RMA2_Port port, RMA2_Region * region);

/*!
    \brief Get the NLA of an address within a registered region
    
    This function returns the network logical address (NLA) of a given
    byte within a registered memory area, a memory region. The NLA is
    suitable for example as destination parameter of put commands

    \param region The region to be queried
    \param offset Offset into the region for which the NLA should be returned
    \param nla Output parameter, caller allocated, will be filled with the NLA of
               the specified address.

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_INV_PORT port was invalid
*/
RMA2_ERROR rma2_get_nla(RMA2_Region* region, size_t offset, RMA2_NLA* nla);

/*!
    \brief Get the Virtual address of a location within a registered region
    
    This function returns the virtual address (VA) of a given
    byte within a registered memory area, a memory region. The VA is
    suitable for local access only.

    \param region The region to be queried
    \param offset Offset into the region for which the VA should be returned
    \param va Output parameter, caller allocated, will be filled with the VA of
               the specified address.

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_INV_PORT port was invalid
*/
  RMA2_ERROR rma2_get_va(RMA2_Region* region, size_t offset, void** va);

/*!
    \brief Get the size of a registered region
    
    This function returns the size of a given region in 
    bytes.

    \param region The region to be queried
    \param size Output parameter, caller allocated, will be filled with the size of
               the specified memory region

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_INV_PORT port was invalid
*/
RMA2_ERROR rma2_get_size(RMA2_Region* region, size_t* size);

/*!
    \brief Get the number of pages in a registered region
    
    This function returns the number of pages that the given region is made of.

    \param region The region to be queried
    \param count Output parameter, caller allocated, will be filled with the page count of
               the specified memory region

    \return \li RMA2_SUCCESS on success
            \li RMA2_ERR_INV_PORT port was invalid
*/
RMA2_ERROR rma2_get_page_count(RMA2_Region* region, size_t* count);

/*!
    \brief Checks wheter the buffer described by start and size is within the region
    
    Returns 1 if buffer is completely inside the region.
*/
int rma2_get_region_contained(RMA2_Region* region, void* start, size_t size);

/*!
    \brief Checks wheter the buffer overlaps with the memory region at least partly
    
    Returns 1 if buffer overlaps with the region at least partly
*/
int rma2_get_region_overlap(RMA2_Region* region, void* start, size_t size);

int rma2_get_region_equal(RMA2_Region* region, void* start, size_t size);
/*!
  @}
*/

/*!
  \defgroup UTIL      Utility functions.

  @{
*/

/*!
    \brief Translate a RMA2_ERROR into a human readable string
  
    \param error The error code to be translated
    \param buf   User supplied buffer
    \param buflen size of the supplied buffer in bytes
*/
void rma2_serror (RMA2_ERROR error, char* buf, int buflen);

/*!
    \brief Prints a human readable error description to stderr
  
    \param error The error code to be printed
    \param header null temrinated string which will be printed first followed by a colon and them the description of error
*/

void rma2_perror(RMA2_ERROR error, char* header);
/*!
    \brief Return the node id.

    This function returns the node id of the local NIC to which the given endpoint
    port is associated to.

    \param port endpoint that is queried
    
    \return local nodeid, RMA2_ANY_NODE if an error occured
*/
RMA2_Nodeid rma2_get_nodeid(RMA2_Port port);

/*!
    \brief Return the virtual process id (VPID).

    This function returns the vpid of the local NIC to which the given endpoint
    port is associated to.

    \param port endpoint that is queried
    
    \return local vpid, RMA2_ANY_VPID if an error occured
*/
RMA2_VPID rma2_get_vpid(RMA2_Port port);

/*!
    \brief Return the maximum number of nodes in the network.

    This function returns the maximum number of nodes in the network.

    \param port endpoint that is queried

    \return node number, 0 if an error occured
*/
RMA2_Nodeid rma2_get_node_num(RMA2_Port port);

/*!
    \brief Return the maximum number of RMA2 processes on each node.

    This function returns the maximum number of RMA2 processes on each node.

    \param port endpoint that is queried

    \return vpid number, 0 if an error occured
*/
RMA2_VPID rma2_get_proc_num(RMA2_Port port);

/*!
    \brief Request to enable interrupt generation for notifications for this virtual connection.
    
    Process must be allowed to use this feature (the kernel driver's choice)

    \param port endpoint of the process
    \param handle virtual connection for which interrupts should be enabled

    \return RMA2_SUCCESS if interrupts are enabled, RMA2_ERROR otherwise
*/
RMA2_ERROR rma2_request_interrupt(RMA2_Port port, RMA2_Handle handle);

/*!
    \brief Return if interrupt generation for notifications is turned on for this virtual connection.

    \param port endpoint that is queried
    \param handle virtual connection to be queried

    \return RMA2_SUCCESS if interrupts are enabled, RMA2_ERROR otherwise
*/
uint32_t rma2_get_interrupt(RMA2_Port port, RMA2_Handle handle);

/*!
    \brief Request to enable remote register access for this virtual connection.
    
    Process must be allowed to use this feature (the kernel driver's choice)

    \param port endpoint of the process
    \param handle virtual connection for which remote register access should be enabled

    \return RMA2_SUCCESS if remote register access is enabled, RMA2_ERROR otherwise
*/
RMA2_ERROR rma2_request_rra(RMA2_Port port, RMA2_Handle handle);
/*!
    \brief Return if remote register access is turned on for this virtual connection.

    \param port endpoint that is queried
    \param handle virtual connection to be queried

    \return RMA2_SUCCESS if remote register access is enabled, RMA2_ERROR otherwise
*/
uint32_t rma2_get_rra(RMA2_Port port, RMA2_Handle handle);

/*!
    \brief Request to enable physical memory access for this virtual connection.
    
    Process must be allowed to use this feature (the kernel driver's choice).
    If this feature is enabled, *all* addresses are interpreted as physical memory references
    by hardware, no translation is performed whatsoever.

    \param port endpoint of the process
    \param handle virtual connection for which physical memory access should be enabled

    \return RMA2_SUCCESS if physical memory access is enabled, RMA2_ERROR otherwise
*/
RMA2_ERROR rma2_request_physical_access(RMA2_Port port, RMA2_Handle handle);

/*!
    \brief Return if physical memory access is turned on for this virtual connection.

   It translation is disabled for this virtual connection, RMA2_SUCCESS is returned, RMA2_ERROR otherwise.

    \param port endpoint that is queried
    \param handle virtual connection to be queried

    \return RMA2_SUCCESS if physical access ois enabled, RMA2_ERROR otherwise
*/
uint32_t rma2_get_physical_access(RMA2_Port port, RMA2_Handle handle);

/*!
    \brief Set Traffic Classes used by transactions on this virtual connection
    
    Process must be allowed to use this feature (the kernel driver's choice).

    \param port endpoint of the process
    \param handle virtual connection for which physical memory access should be enabled
    \param request_tc Traffic class to be used for requests
    \param response_tc Traffic class to be used for responses    

    \return RMA2_SUCCESS if physical memory access is enabled, RMA2_ERROR otherwise
*/
RMA2_ERROR rma2_request_tcs(RMA2_Port port, RMA2_Handle handle,uint32_t request_tc, uint32_t response_tc);

/*!
    \brief Return the traffic class used by this virtual connection.
    
    There is one TC used for requests and one for responses. Return them for this virtual connection.

    \param port endpoint that is queried
    \param handle virtual connection to be queried
    \param request_tc Traffic class used for requests
    \param response_tc Traffic class used for responses

    \return RMA2_SUCCESS 
*/
RMA2_ERROR rma2_get_tcs(RMA2_Port port, RMA2_Handle handle,uint32_t* request_tc, uint32_t* response_tc);

// FIXME: add protection id function
/*!
    \brief Set the protection id of this endpoint
    
    All processes of one group, i.e. that communicate with each other, should set the same protection id.
    Packets with a wrong protection id are dropped by hardware.

    \param port endpoint of the process
    \param id the new value for the protection id for this endpoint

    \return RMA2_SUCCESS
*/
RMA2_ERROR rma2_set_protectionid(RMA2_Port port, uint16_t id);

/*!
  \brief Returns the size of the notification queue of the given endpoint in bytes
  
  One notification has a size of 128-bit=32byte, thus the number of notifications that can be stored in the queue
  can be calculated by dividing the size by 32.
 
  \param port endpoint of the process 
  \return		size of the queue in bytes
*/
uint32_t rma2_get_queue_size(RMA2_Port port);

/*!
  \brief Sets the size of the notification queue of the given endpoint in bytes
  
  This function should be called before any traffic is generated. The size can only be changed in multiples of 4kb (pages).
  If the size cannot be changed (for example out of memory), the size remains unchanged and an error is returned.
    
   \param port endpoint of the process
   \param bytes     new size of the receive queue in bytes
   \return RMA2_SUCCESS if successfull, Error code otherwise
*/
RMA2_ERROR rma2_set_queue_size(RMA2_Port port, uint32_t bytes);

/*!
  \brief Returns the size of the notification queue segments of the given endpoint in bytes
  
  A notification queue can be constructed from multiple segments, to limit the impact on the virtual memory subsytem of the OS.
  This is only interesting for advanced uses.
 
  \param port endpoint of the process
  \return		size of the queue
*/
uint32_t rma2_get_queue_segment_size(RMA2_Port port);

/*!
  \brief Sets the size of the notification queue segments of the given endpoint in bytes

  A notification queue can be constructed from multiple segments, to limit the impact on the virtual memory subsytem of the OS.
  This is only interesting for advanced uses.
  This function should be called before any traffic is generated. The segment size can only be changed in multiples of 4kb (pages).
  If the segment size cannot be changed (for example out of memory), the size remains unchanged and an error is returned.
 
  \param port endpoint of the process
  \param size the new size of the notification queue segments in byte
  \return		segment size of the queue
*/
RMA2_ERROR rma2_set_queue_segment_size(RMA2_Port port,uint32_t size);

/*!
 * \brief Manually drain the replay buffer
 

 If the EXTOLL device is under heavy load, and a message is written to the device, a CPU stall will happen.
 To prevent this, LibRMA2 features a replay buffer.
 If the device is busy, messages are written to the buffer and executed later.
 By default, the buffer is checked for outstanding messages on all possible occasions and no user interaction is required.
 If more controle over this buffer is desired, it's behaviour can be altered with the following three functions.

\param port the RMA2 Port which replay buffer should be drained
 */
void rma2_replay_buffer_drain(RMA2_Port port);

/*!
 *\brief Set the mode of the replay buffer

 \param port the port which replay buffer's options are to be altered
 \param mode new replay buffer mode forautomatic draining
 */
void rma2_set_replay_mode(RMA2_Port port, RMA2_Replay_Buffer_Mode mode);

/*!
 * \brief Get the current mode of the replay buffer
 *
 * \param port the port which replay buffers status is aked
 * \return the ports replay buffer modeis asked, encoded as RMA2_Replay_Buffer_Mode
 */
RMA2_Replay_Buffer_Mode rma2_get_replay_buffer_mode(RMA2_Port port);

/*!
  @}
*/

/*!
  @}
*/
#ifdef  __cplusplus
}
#endif

#endif // LIBRMA2_H
