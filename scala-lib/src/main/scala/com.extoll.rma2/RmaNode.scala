package com.extoll.rma2

import uni.hd.cag.osys.rfg.rf.RegisterFileHost
import uni.hd.cag.osys.rfg.rf.RegisterFile
 
/**
 * Base trait to mark a node object as RMA accessible
 * 
 * Contains some basic infos needed, like the node id
 */
trait RmaNode extends RegisterFileHost {
  

  
}

class DummyRmaNode(  var id : Short,var registerFile : RegisterFile = null) extends RmaNode {
  

  
} 