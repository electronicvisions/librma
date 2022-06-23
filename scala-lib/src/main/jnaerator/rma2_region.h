/*
*  This file is part of the  Linux Kernel Driver Package for EXTOLL R2.
*
*  Copyright (C) 2011 EXTOLL GmbH
*
*  Written by Mondrian Nuessle (2011)   
*
*  Written by Mondrian Nuessle (2011)
*  based on the work of Martin Scherer (2008)
*  based on the work of Philipp Degler (2007)
*  based on the work of Mondrian Nuessle (2006)
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef _RMA2_RMA2_REGION_H_
#define _RMA2_RMA2_REGION_H_

/******************************************************************************
*  RMA library data structures                                                *
******************************************************************************/
/*!
   \brief The RMA2_Region represents a registered memory region
    
    In EXTOLL RMA2, memory must be registered before it can be used
    for put or get transactions. The result of registering a region of
    memory is an instance of RMA2_Region.
    
    This data type is used both in user space and in kernel space. Thus
    it lives in its own header file.
*/
typedef struct _region {
    void* start; //!< virtual start address of the registered region
    unsigned long nla; //!< array containing the NLAs of the registered region
    uint32_t count; //!< number of pages, also number of NLAs
    uint32_t offset; //!< offset this region starts within the first page

    void* end; //!< virtual end address of the registered region
    size_t size; //!< size of the virtual region

  /* Code has been deprecated. 2010, MN
	union {
    uint64_t global; //! Either globally, both refcounts
    struct { 
      uint32_t local; //! or only the local registrations
      uint32_t remote; //! or only remote registrations
    } div;
  } refcount; //!union for refcounter, only used by registration manager

  struct _region* next; //! next pointer, only used by registration manager
  struct _region* previous;//! previous pointer, only used by registration manager
  struct _region* child;//! child pointer, only used by registration manager
  struct _region* overlap;//! overlap pointer, only used by registration manager
  uint32_t overlap_idx;//! overlap index, only used by registration manager
  */
}  RMA2_Region;

#endif
