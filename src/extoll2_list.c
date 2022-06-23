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

/* IMPORTANT: All items must have a list_head_t as *first* member!
 */

#include "extoll2_list.h"

#include <stdlib.h>
#include <stdio.h>

//#define DEBUG(a) a
#define DEBUG(a)


extoll2_list_t* extoll2_construct_list(uint32_t item_size, uint32_t initial_pool_size,uint32_t free_increment,const char* name)
{
  extoll2_list_t* list=malloc(sizeof(extoll2_list_t));
  list->item_size=item_size;
  list->num_items=0;
  list->items_alloced=0;
  list->free_increment=free_increment;
  //EXTOLL_INIT_LIST_HEAD(&(list->pool_list));
  list->pool_list.next = &(list->pool_list); 
  list->pool_list.prev = &(list->pool_list);
  //EXTOLL_INIT_LIST_HEAD(&(list->free_list));
  list->free_list.next = &(list->free_list); 
  list->free_list.prev = &(list->free_list);
  //EXTOLL_INIT_LIST_HEAD(&(list->list));
  list->list.next = &(list->list); 
  list->list.prev = &(list->list);
  
  extoll2_alloc_pool(list,initial_pool_size);
  list->num_free_items=initial_pool_size;
  strncpy(list->name,name,32);
  return list;
}

void extoll2_destruct_list(extoll2_list_t* list)
{
  extoll_list_head_t* temp;
  extoll_list_head_t* itemp;
  //extoll_list_for_each_safe(itemp, temp, &(list->free_list)) 
  for (itemp = (&(list->free_list))->next, temp = itemp->next; itemp != (&(list->free_list)); \
		itemp = temp, temp = itemp->next)
  {
      //extoll_list_del(itemp);
      itemp->next->prev = itemp->prev;
      itemp->prev->next =  itemp->next;
  }
/*  extoll_list_for_each_safe(itemp, temp, &(list->list)) {
      extoll_list_del(itemp);
  }*/
  for (itemp = (&(list->list))->next, temp = itemp->next; itemp != (&(list->list)); \
		itemp = temp, temp = itemp->next)
  {
      //extoll_list_del(itemp);
      itemp->next->prev = itemp->prev;
      itemp->prev->next =  itemp->next;
  }
/*  extoll_list_for_each_safe(itemp, temp, &(list->pool_list)) {
      extoll2_free_pool((extoll2_pool_item_t*)(itemp));
  }*/
  for (itemp = (&(list->pool_list))->next, temp = itemp->next; itemp != (&(list->pool_list)); \
		itemp = temp, temp = itemp->next)
  {
      //extoll_list_del(itemp);
      itemp->next->prev = itemp->prev;
      itemp->prev->next =  itemp->next;
  }

 free(list);
  
}

int extoll2_alloc_pool(extoll2_list_t* list,uint32_t num_items)
{
  int i;
  extoll2_pool_item_t *pool=malloc(sizeof(extoll2_pool_item_t));
  extoll_list_head_t* item;
  
  //printf("alloc_pool called with num_items=%u\n",num_items);
  pool->entries=num_items;
  pool->size=num_items*list->item_size;
  pool->address=malloc(pool->size);
  memset(pool->address,0,pool->size);
  //extoll_list_add_tail(  (&(pool->list)),&(list->pool_list));
  list->pool_list.prev       = (&(pool->list));
  list->pool_list.prev->next = (&(pool->list));
  (&(pool->list))->next      = &(list->pool_list);
  (&(pool->list))->prev      = &(list->pool_list);

      
  for (i=0;i<num_items;i++)
  {
    item=pool->address+i*list->item_size;
    //extoll_list_add_tail(item,&(list->free_list));
    extoll2_free_item(list,item);
  }
  list->items_alloced+=num_items;
  //list->num_free_items+=num_items;
  return 0;
}

int extoll2_free_pool(extoll2_pool_item_t * pool)
{
  //extoll_list_del( (&(pool->list)) );
  //extoll_list_del(itemp);
 pool->list.next->prev = pool->list.prev;
 pool->list.prev->next = pool->list.next;
      
  free(pool->address);
  free(pool);
  return 0;
}

