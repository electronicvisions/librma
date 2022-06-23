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
#include <string.h>
#include <rma2.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#define PORT_COUNT 1
struct timeval start,stop;

void print_err(RMA2_ERROR err)
{
  fprintf(stderr, "RMA error occured: %u\n", (unsigned int )err);
}

int main(int argc, char** argv)
{
  long number,number2;
  long i;
  
  RMA2_Port port;

  uint64_t max_size;

  RMA2_Region** regions;
  
  void** buffers;
  uint64_t* sizes;

  RMA2_ERROR rc;

  double sec;
  uint64_t page_count, pages;
  
  uint64_t random_size;

  long j;

  if (argc!=5) 
    {
      printf("Usage: ./testregister <number of registrations> <max size> <inner_loop> <random (1) sizes or not (0)> \n");
      abort();
    }

  number=strtol(argv[1],0,0);
  max_size=strtol(argv[2],0,0);
  number2=strtol(argv[3],0,0);
  random_size=strtol(argv[4],0,0);

  regions=malloc(sizeof(RMA2_Region*) * number);
  buffers=malloc(sizeof(void*) * number);
  sizes=malloc(sizeof(uint64_t)*number);
  page_count=0;
  for (i=0;i<number;i++)
    {
	  if (random_size>0)
		sizes[i]= (rand() % (max_size-1))+1;	  
	  else
		sizes[i]=max_size;
      //sizes[i]=4096;
      pages=((sizes[i] - 1) >> 12) + 1;
      printf("Region %d has size %lu (%lu pages).\n",i,sizes[i],pages);
      page_count+=pages;
    }  
  printf("Allocating memory...\n");
  for (i=0;i<number;i++)
    {
      buffers[i]=malloc(sizes[i]);
      if (buffers[i]==0)
        {
          perror("malloc:");
          return -1;
        }
      memset(buffers[i], 0, sizes[i]);
      printf("Region [%d] starts at %p\n",i,buffers[i]);
    }
  printf("Opening port(s)\n");
  for (i=0;i<PORT_COUNT;i++)
  {
  rc=rma2_open(&port);
  if (rc!=RMA2_SUCCESS) print_err(rc);
  }
  printf("Now starting to (un-)register memory\n");
  for (j=0;j<number;j++)
    {
      gettimeofday(&start,0);
      for (i=0;i<number2;i++)
        {
          rc=rma2_register(port,buffers[i], sizes[i], &(regions[i]));
          if (rc!=RMA2_SUCCESS) {
	    print_err(rc);
	    abort();
          }
        }
      gettimeofday(&stop,0);
      sec = (1.0)*(stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec)*0.000001;
      
      printf("Registering of %lu pages done, took %lf s, one page on avg. %lf us.\n", page_count, sec, sec / (page_count*1.0) *1000000.0);
      printf("Now unregistering again.\n");
      gettimeofday(&start,0);
      for (i=0;i<number2;i++)
        {
          rc=rma2_unregister(port, regions[i]);
          if (rc!=RMA2_SUCCESS) {
            print_err(rc);
            abort();
          }
        }
      gettimeofday(&stop,0);
      sec = (1.0)*(stop.tv_sec - start.tv_sec) + (stop.tv_usec - start.tv_usec)*0.000001;
      //printf("%d, %d, %d, %d\n",stop.tv_sec,start.tv_sec, stop.tv_usec,start.tv_usec);
      printf("Unregistering of %lu pages done, took %lf s, one page on avg. %lf us.\n", page_count, sec, sec / (page_count*1.0) *1000000.0);
    }
  
  rc=rma2_close(port);
  if (rc!=RMA2_SUCCESS) print_err(rc);
  free(regions);
  for (i=0;i<number;i++)
    {
      free(buffers[i]);
    }
  free(buffers);
  free(sizes);
  return 0;
}
