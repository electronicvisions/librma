#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#define __USE_GNU
#include <dlfcn.h>

#include <assert.h>
#include "rma2.h"
#include <rma2_ioctl.h>

#define RMA2CACHED

//comment/uncomment to enable/disable debug output
#define DEBUG(a)
//#define DEBUG(a) a


/* FREE and MUNMAP intercetion using dynamic linking tricks and weak symbols (libc) vs strong symbols (librma2).
 * This may not work on every system.
 * Inspired by source code from mvapich2
 * 
 * Interception done to implement registration caching.
 */

typedef int (*munmap_t)(void*, size_t);
typedef int (*free_t)(void*);

typedef struct {
    munmap_t    munmap;
    free_t      free;
    free_t      our_free;
    int         cache_count; //!size of the cache
    int        	print_stats; //!whether to print cache usage statistics at the end
    uint64_t    register_misses;  //! region cache miss
    uint64_t    register_hits;     //! region cache hit
    uint64_t    register_replace; //! region that is still valid but unused got replaced
    uint64_t    register_fill; //! found empty slot that was used
    uint64_t    register_free; //! how many times a region had to be really unregistered because of free/unmap
    uint64_t    register_free_not_free; //! how many times a region should be really unregistered because of free/unmap but was in use
    uint64_t    register_uncached; //! count region registers that are not cached at all (cache full and no victims available)
    uint64_t    register_use_child; //! count of cache misses that used a different sub-region than the original registration
    RMA2_Region** region;
} librma2_malloc_info_t;

librma2_malloc_info_t librma2_minfo;



static void
rma2_free_handler(void* address, size_t length)
{
#ifndef RMA2CACHED
  return;
#else  
  int i;
  int result;
  RMA2_Region* current;
  
  //check whether the freed address is within a cached region
  for (i=0;i<librma2_minfo.cache_count;i++)
  {
    //printf("checking region %d\n",i);
    current=librma2_minfo.region[i];
    if (current==NULL) continue; //slot is empty
    //printf("checking intersect\n");
    if (length!=0)
      result=rma2_get_region_overlap(current,address,length);
    else{ 
      DEBUG(printf("intersect check for a free of size %ld at address %p\n",((uint64_t*)address)[-1] & ~0x3l, address);)
      result=rma2_get_region_overlap(current,address,((uint64_t*)address)[-1] & ~0x3l /*~0xfl*/ );
    }
    DEBUG(printf("now checking result... with current=%p\n",current);)
    if (result==1) { 
      assert(current!=NULL);
      DEBUG(printf("refcount %ld\n",current->refcount);)
      if (current->refcount==0) //not in use at the moment -> can be safely removed!
        {
          DEBUG(fprintf(stderr,"%d: unregistered region %d\n",((RMA2_Port)current->endpoint)->vpid,i);)
          //printf("current->endpoint: %p\n",current);
          rma2_unregister( (RMA2_Port)current->endpoint, current);  //unregister region
          librma2_minfo.region[i]=NULL; //and set cache slot to zero
          librma2_minfo.register_free++;      
        }
       else
       {
         //well don't do anything for the moment
          DEBUG(printf("!!!!!!! %d: free within a registered region, which is in use. Leaving region there. start %p length %ld region->start %p region->end %p\n",((RMA2_Port)current->endpoint)->vpid,
           start,length,region->start,region->end);)
         librma2_minfo.register_free_not_free++;
       }
       //printf("free handler done\n");
       //return;
    }
  } 
  //printf("free handler done\n");
#endif  
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
RMA2_ERROR rma2_register_cached(RMA2_Port port, void* address, size_t size, RMA2_Region** region)
{
#ifndef RMA2CACHED  
  return rma2_register(port,address,size,region);
#else  
  int i;
  int result=0;
  RMA2_ERROR rma2_err;
  RMA2_Region* current;
  
//  assert(librma2_minfo.our_free==free);
  
  for (i=0;i<librma2_minfo.cache_count;i++)
  {
    current=librma2_minfo.region[i];
    if (current==NULL) continue; //slot is empty
    result=rma2_get_region_contained(current,address,size);    
    if (result==1) /*&& current->start+current->offset==address )*/ {
      DEBUG(printf("found region %d\n",i);)
      break;
    } else result=0;
  }
  if (result==1 /*&& current->start+current->offset==address && current->size==size*/) //found an intersecting region
  {
     //mark as used
     current->refcount++;

     // fix offset...?
     if (current->start+current->offset!=address) {
       
        //printf("registering child region!\n");   
        RMA2_Region* region_t=(RMA2_Region*)extoll2_new_item(port->region_queue);
        if (region==0)
        {
          perror("rma2_register malloc:");
          return RMA2_ERR_ERROR;
        }
        memcpy(region_t,current,sizeof(RMA2_Region));
        region_t->offset=address-current->start;
        region_t->parent=current;
        DEBUG(printf("registering child region! org start %p org offset %d org end %p, new offset %d new address %p new size %ld\n", current->start, current->offset, current->end, region_t->offset, address,size); )
        assert(region_t->start==current->start);
        assert(region_t->start+region_t->offset+size <= current->end);
        current=region_t;
        librma2_minfo.register_use_child++;
     }
     *region=current;
     librma2_minfo.register_hits++;
     return RMA2_SUCCESS;
  }
  DEBUG(printf("registering new region\n");)
  librma2_minfo.register_misses++;
  //invalid entry?
  for (i=0;i<librma2_minfo.cache_count;i++)
  {
    current=librma2_minfo.region[i];
    DEBUG(printf("region [%d] is %p size is  %ld\n",i,current,size);)
    if (current==NULL) { //empty, can be set!
         //printf("FILL!!\n");
         result=1;
         rma2_err = rma2_register(port,address,size,&(librma2_minfo.region[i]));
         if (rma2_err!=RMA2_SUCCESS) {
           printf("error registering!\n");
           return rma2_err;
         }
         //printf("registered\n");
         *region=librma2_minfo.region[i];
         //printf("set *region to %p!\n",librma2_minfo.region[i]);
         (librma2_minfo.region[i])->refcount=1;
         librma2_minfo.register_fill++;
         //printf("done\n");
         return rma2_err;
    }
  }
  DEBUG(printf("no invalid entries\n");)
  //unused entry?
  for (i=0;i<librma2_minfo.cache_count;i++)
      {
        current=librma2_minfo.region[i];
        if (current->refcount==0) //can replace!
        {
          result=1;
          rma2_err=rma2_unregister(port, current);
          rma2_err |= rma2_register(port,address,size,&(librma2_minfo.region[i]));
          *region=librma2_minfo.region[i];
          librma2_minfo.region[i]->refcount=1;
          librma2_minfo.register_replace++;
          return rma2_err; 
        }
      }
   DEBUG(printf("no entry available\n");)
   //still not found -> register uncached!
   rma2_err = rma2_register(port,address,size,region);
   librma2_minfo.register_uncached++;
   return rma2_err;
#endif   
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
RMA2_ERROR rma2_unregister_cached(RMA2_Port port,RMA2_Region* region)
{
#ifndef RMA2CACHED
 return rma2_unregister(port,region);
#else 
  int i;
  RMA2_Region* current;
  //first check for leight weight, i.e. child, region
  if (region->parent!=NULL)
  {
    DEBUG(printf("deregistering child region!\n");)
    region->parent->refcount--; //dec refcount of parent
    extoll2_free_item(port->region_queue,(extoll_list_head_t* )region);   // and return the child item to the free list
    return RMA2_SUCCESS;
  }
 
  //check whether we have cached that
  for (i=0;i<librma2_minfo.cache_count;i++)
  {
    current=librma2_minfo.region[i];
    if (region==current)
    {
      current->refcount--;
      assert(current->refcount>=0);
      return RMA2_SUCCESS;
    }
  }
  //not cached, just unregister
  //fprintf(stderr,"uncached unregister\n");
  return rma2_unregister(port, region);  
#endif  
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
static void __set_real_munmap_ptr()
{
    munmap_t munmap_ptr = (munmap_t) dlsym(RTLD_NEXT, "munmap");
    char* dlerror_str = dlerror();
    if(NULL != dlerror_str) {
        fprintf(stderr,"Error resolving munmap(): %s\n", dlerror_str);
    }

    /*
     * The following code tries to detect link error where both static 
     * and dynamic libraries are linked to executable. This link error 
     * is not currently detected by the linker (should it be? I don't know).
     * However, at execution, it produces an infinite recursive loop of 
     * librma2_munmap() -> munmap() -> librma2_munmap() -> ...
     * that crashes the program. 
     * It is because in this case, the above code picks the wrong munmap() 
     * function from the second library instead of the one from the system.
     */

    void* handle = dlopen("librma2rc.so", RTLD_LAZY | RTLD_LOCAL);
    dlerror_str = dlerror();
    if(NULL != dlerror_str) {
        // The error in this case can be ignored
        // This is probably because only the shared library is not available. 
        // However, we keep calling dlerror() so it reset the error flag for dl calls.
    }

    if (NULL != handle) {
        /* Shared libraries are in use, otherwise simply proceed. */
        munmap_t librma2_munmap_ptr = (munmap_t) dlsym(handle, "munmap");
        char* dlerror_str = dlerror();
        if(NULL != dlerror_str) {
            fprintf(stderr,"Error resolving munmap() from librma2.so: %s\n", dlerror_str);
        }

        if (munmap_ptr == librma2_munmap_ptr) {
            /*
             * This means the "real" munmap is the same as librma2.so.
             * This happens if a program is statically and dynamically
             * linked at the same time.  Using the libmpich.so munmap
             * again results in recursion.
             */
            fprintf(stderr,"Error getting real munmap(). LIBRMA2 cannot run properly.\n");
            fprintf(stderr,"This error usually means that the program is linked with both static and shared LIBRMA2 libraries.\n");
            fprintf(stderr,"Please check your Makefile or your link command line.\n");
            exit(-1);
        }
    }

    librma2_minfo.munmap = munmap_ptr;
}
#pragma GCC diagnostic pop

int munmap(void *addr, size_t length)
{
  int result;
  DEBUG(fprintf(stderr,"*********** our munmap!\n");)
  if (librma2_minfo.munmap==NULL) {
    fprintf(stderr,"WARNING: librma2 overriding of munmap(), real munmap not set yet. You will have a memory leak.\n");
    return 0;
  }
  
  rma2_free_handler(addr,length);
  DEBUG(printf("doing system munmap\n");)
  result=librma2_minfo.munmap(addr,length);
  DEBUG(printf("system munmap returned %d\n",result);)
  
  return result;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
static void __set_real_free_ptr()
{
    free_t free_ptr = (free_t) dlsym(RTLD_NEXT, "free");
    char* dlerror_str = dlerror();
    if(NULL != dlerror_str) {
        fprintf(stderr,"Error resolving free(): %s\n", dlerror_str);
    }

    /*
     * The following code tries to detect link error where both static 
     * and dynamic libraries are linked to executable. This link error 
     * is not currently detected by the linker (should it be? I don't know).
     * However, at execution, it produces an infinite recursive loop of 
     * librma2_free() -> free() -> librma2_free() -> ...
     * that crashes the program. 
     * It is because in this case, the above code picks the wrong munmap() 
     * function from the second library instead of the one from the system.
     */

    void* handle = dlopen("librma2rc.so", RTLD_LAZY | RTLD_LOCAL);
    dlerror_str = dlerror();
    if(NULL != dlerror_str) {
        // The error in this case can be ignored
        // This is probably because only the shared library is not available. 
        // However, we keep calling dlerror() so it reset the error flag for dl calls.
    }

    if (NULL != handle) 
    {
        /* Shared libraries are in use, otherwise simply proceed. */
        free_t librma2_free_ptr = (free_t) dlsym(handle, "free");
        char* dlerror_str = dlerror();
        if(NULL != dlerror_str) {
            fprintf(stderr,"Error resolving free() from librma2.so: %s\n", dlerror_str);
        }

        if (free_ptr == librma2_free_ptr) {
            /*
             * This means the "real" free is the same as librma2.so.
             * 
             * This happens if a program is statically and dynamically
             * linked at the same time.  Using the librma2.so free
             * again results in recursion.
             */
            fprintf(stderr,"Error getting real free(). LIBRMA2 cannot run properly.\n");
            fprintf(stderr,"This error usually means that the program is linked with both static and shared LIBRMA2 libraries.\n");
            fprintf(stderr,"Please check your Makefile or your link command line.\n");
            exit(-1);
        }
    }

    librma2_minfo.free = free_ptr;    
    DEBUG(printf("Out free is at %p , libc free is at %p\n",free, free_ptr);)
}
#pragma GCC diagnostic pop



void free(void* ptr)
{
  
  //if (ptr==0) printf("free called with NULL\n");
  //else printf("free called with NOT NULL\n");
  DEBUG(fprintf(stderr,"our free called\n");)
  
  if (librma2_minfo.free==NULL){
   //fprintf(stderr,"WARNING: librma2 overriding of free(), real free() not set yet. You will have a memory leak.\n");
    return;
  }
  //printf("called with %p\n",ptr);
  //assert(ptr!=NULL);
  if (ptr!=0) {
    rma2_free_handler(ptr,0);
  } 
  librma2_minfo.free(ptr);
//   else {
//     printf("system free called with NIL!\n");
//     //abort();
//   }
}

char* __extoll2_rma_rc;


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
__attribute__((constructor)) void __rma2rc_lib_init(void) {
  int i;
  //printf("librma2 is loaded\n");
  
  __extoll2_rma_rc=getenv("EXTOLL2_RMA_RC");
  if (__extoll2_rma_rc==NULL) { //default behavior
    librma2_minfo.cache_count=256;
  } else
  {
    librma2_minfo.cache_count=strtol(__extoll2_rma_rc,0,0);    
  }
  librma2_minfo.region=malloc(librma2_minfo.cache_count*sizeof(RMA2_Region*));
	
 	if (getenv("EXTOLL2_RMA_RC_STATS")!=NULL)
	{
		librma2_minfo.print_stats=1;
	}
	else
	{
		   librma2_minfo.print_stats=0;
	}
  
  //check for the malloc stuff!
 __set_real_free_ptr();
 __set_real_munmap_ptr();
 
 
 librma2_minfo.our_free=(free_t)free;
 librma2_minfo.register_misses=0l;  //! region cache miss
 librma2_minfo.register_hits=0l;     //! region cache hit
 librma2_minfo.register_replace=0l; //! region that is still valid but unused got replaced
 librma2_minfo.register_fill=0l; //! found empty slot that was used
 librma2_minfo.register_free=0l; //! how many times a region had to be really unregistered because of free/unmap
 librma2_minfo.register_free_not_free=0l; //! how many times a region should be really unregistered because of free/unmap but was in use
 librma2_minfo.register_uncached=0l; //! count region registers that are not cached at all (cache full and no victims available)
 librma2_minfo.register_use_child=0l;
 for (i=0;i<librma2_minfo.cache_count;i++)
   librma2_minfo.region[i]=0;

 //printf("malloc is at %p\n",malloc);
}
#pragma GCC diagnostic pop

__attribute__ ((destructor)) void __rma2rc_lib_fini(void) {
  if (librma2_minfo.print_stats==1) {
  printf("\nRegion Cache Characteristics: using %d entries\n"
         "------------------------------------------------------------\n"
         "%lf Cache Hit ratio\n\n"
         "%ld cache hits\n"
         "%ld cache hits to a sub-region (required sub-region handling)\n"
         "%ld cache misses\n"
         "%ld cache replacements\n"
         "%ld cache fills\n"
         "%ld cache items unregistered and freed\n"
         "%ld cache items should be unregistered but were in use\n"
         "%ld registrations that could not be cached\n\n",
         librma2_minfo.cache_count,
         ((double)librma2_minfo.register_hits)/((double)(librma2_minfo.register_hits+librma2_minfo.register_misses)),
         librma2_minfo.register_hits,
         librma2_minfo.register_use_child,
         librma2_minfo.register_misses,         
         librma2_minfo.register_replace,
         librma2_minfo.register_fill,
         librma2_minfo.register_free,
         librma2_minfo.register_free_not_free,
         librma2_minfo.register_uncached);
	}
  free(librma2_minfo.region);
}
