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

#include <stdio.h>
#include <stdlib.h>
#include <rma2.h>

int main(int argc, char** argv)
{
  RMA2_Port port;
  RMA2_ERROR rc;
  RMA2_VPID vpid;
  RMA2_Nodeid nodeid;
  RMA2_Handle handle;
  RMA2_VPID dest_vpid;
  RMA2_Nodeid dest_nodeid;
  RMA2_Connection_Options conn_type;
  
  if (argc<4)
  {
    fprintf(stderr,"Erroneous commandline. Must provide 3 parameter: dest. node, dest. vpid and connection type\n");
    abort();
  }
  dest_nodeid=atol(argv[1]);
  dest_vpid=atol(argv[2]);
  conn_type=atol(argv[3]);
    
  printf("Connection to node %d, vpid %d requested. Using connection type of %x\n", dest_nodeid,dest_vpid, conn_type);
  
  rc=rma2_open(&port);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA open failed (%d)\n",rc);
    abort();
  }
  
  nodeid=rma2_get_nodeid(port);
  vpid=rma2_get_vpid(port);
  printf("opened port %d on node %d\n",vpid, nodeid);
  
  rc=rma2_connect(port,dest_nodeid, dest_vpid ,conn_type, &handle);
  if (rc!=RMA2_SUCCESS)
  {
    fprintf(stderr,"RMA connect failed (%d)\n",rc);
    abort();
  }
  printf("connected!\n");
  rc=rma2_disconnect(port,handle);
  rma2_close(port);
  return 0;
}