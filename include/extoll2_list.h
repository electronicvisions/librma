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

/* append -> opal_list_append
remove -> opal_list_remove_item
is_empty ->opal_list_is_empty
first -> opal_list_get_first
last -> opal_list_get_last
get_next ->opal_list_get_next

construct_list -> alloc a list mgt struct, alloc pool
destruct_list -> remove all elements from list, free pool
new_item -> OBJ_RELEASE -> if free_list not empty, remove from free_list and return; else alloc_pool, then retry
free_item -> OBJ_NEW -> move to free_list (must be removed from list!->assertion!)

alloc_pool -> alloc a number of items and add them to free_list, record pool address in pool list in mgt_struct
free_pool  -> free pool (how to do dynamic??)
*/

/* IMPORTANT: All items must have a extoll_list_head_t as *first* member!
 */
#ifndef EXTOLL2_LIST
#define EXTOLL2_LIST

#include <stdint.h>
#include <string.h>
//#include "extoll2_internal_list.h"
// #include <arch_define.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

/*
 * These are non-NULL pointers that will result in page faults
 * under normal circumstances, used to verify that nobody uses
 * non-initialized list entries.
 */
#define EXTOLL_LIST_POISON1  ((void *) 0x00100100)
#define EXTOLL_LIST_POISON2  ((void *) 0x00200200)

struct extoll_list_head {
	struct extoll_list_head *next, *prev;
};
typedef struct extoll_list_head extoll_list_head_t;

typedef struct _extoll2_pool_item {
  extoll_list_head_t list; //list
  void* address; //pointer to allocated memory (for use with free())
  uint32_t size; //in bytes
  uint32_t entries; //!number of entries allocated
} extoll2_pool_item_t;


typedef struct _extoll2_list_mgt{
  char     name[32];
  uint32_t item_size; //!size of one item used in the list
  uint32_t num_items; //!number of items currently in the list, i.e. the "size" of the list
  uint32_t free_increment; //!how many items shall be allocated in one pool
  uint32_t num_free_items;
  uint32_t items_alloced;
  extoll_list_head_t pool_list; //!list of allocated pools
  extoll_list_head_t free_list; //!list of free items
  extoll_list_head_t list; //!actual list
} extoll2_list_t;


extern extoll2_list_t* extoll2_construct_list(uint32_t item_size, uint32_t initial_pool_size,uint32_t free_increment, const char* name);

extern void extoll2_destruct_list(extoll2_list_t* list);

static inline uint32_t extoll2_list_get_size(extoll2_list_t* list)
{
  return list->num_items;
}

static inline  void extoll2_list_append(extoll2_list_t* list, extoll_list_head_t* itemp)
{
  list->num_items++;
  //printf("list now %d\n",list->num_items);
  //extoll_list_add_tail(itemp,&(list->list));
  //__extoll_list_add(itemp, list->list.prev, &(list->list));

  itemp->next = &(list->list);
  itemp->prev = list->list.prev;
  list->list.prev->next = itemp;
  list->list.prev = itemp;

}

static inline  void extoll2_list_remove(extoll2_list_t* list, extoll_list_head_t* itemp)
{
 list->num_items--;
 //extoll_list_del(itemp);
 //__extoll_list_del(itemp->prev, itemp->next);
 itemp->next->prev = itemp->prev;
 itemp->prev->next =  itemp->next;
 
 itemp->next = ( extoll_list_head_t*)EXTOLL_LIST_POISON1;
 itemp->prev = ( extoll_list_head_t*)EXTOLL_LIST_POISON2;
}

static inline  int extoll2_list_isempty(extoll2_list_t* list)
{
  return (list->num_items==0);
  //return (list->list.next == &(list->list));
  //return extoll_list_empty(&(list->list));
}

static inline  extoll_list_head_t* extoll2_list_first(extoll2_list_t* list)
{
  return list->list.next;
}

static inline  extoll_list_head_t* extoll2_list_last(extoll2_list_t* list)
{
   return list->list.prev;
}

static inline  extoll_list_head_t* extoll2_list_get_next(extoll2_list_t* list,extoll_list_head_t* itemp)
{
  (void)list;
  return itemp->next;
}

extern int extoll2_alloc_pool(extoll2_list_t* list,uint32_t num_items);

extern int extoll2_free_pool(extoll2_pool_item_t * pool);

static inline uint32_t extoll2_list_get_freelist_size(extoll2_list_t* list)
{
  return list->num_free_items;
}

static inline uint32_t extoll2_list_get_allocated_items(extoll2_list_t* list)
{
  return list->items_alloced;
}

static inline extoll_list_head_t* extoll2_new_item(extoll2_list_t* list)
{
  int result;
  extoll_list_head_t* item;
  //if (unlikely(extoll_list_empty(&(list->free_list))))
  if (unlikely(list->free_list.next == &(list->free_list)))
  {
    //printf("extend pool of list %s\n",list->name);
    result=extoll2_alloc_pool(list,list->free_increment);
    if (result!=0) return 0;
  }
  item=list->free_list.next;
  list->free_list.next=item->next;
  list->free_list.next->prev=item->prev;
  //extoll_list_del_init(item); //remove entry from free_list front
  //__extoll_list_del(item->prev, item->next);
  //item->prev->prev = item->prev;
  //item->prev->next = item->prev;
  //EXTOLL_INIT_LIST_HEAD(item);
  item->next = item; 
  item->prev = item;
  
  list->num_free_items--;
  return item;
}

static inline  int extoll2_free_item(extoll2_list_t* list,extoll_list_head_t* item)
{
  //extoll_list_add_tail(item, &(list->free_list));  
  item->next = &(list->free_list);
  item->prev = list->free_list.prev;
  list->free_list.prev->next = item;
  list->free_list.prev = item;
 
  list->num_free_items++;
  return 0;
}

#endif
