/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <gmp.h>
#include <mpc.h>
#include <stddef.h>

#include "deque.h"
#include "vmcommon.h"
#include "xalloc.h"

#define resource_list_destroy(id, list)	resource_list_destroy_##id (list)
#define ENTRY(id, type, init, destroy)			\
  static void						\
  resource_list_destroy_##id (ResourceList(id) *deque)	\
  {							\
    Resource(id) *res;					\
    while ((res = deque_pop (deque)) != NULL)		\
      {							\
	destroy (res->payload);				\
	free (res);					\
      }							\
  }
RESOURCES
#undef ENTRY

void
resource_manager_init (ResourceManager *rm)
{
#define ENTRY(id, type, init, destroy)		\
  deque_init (&rm->nursery_list(id));		\
  deque_init (&rm->heap_list(id));		\
  deque_init (&rm->free_list(id));		
  RESOURCES
#undef ENTRY
}

void
resource_manager_destroy (ResourceManager *rm)
{
#define ENTRY(id, type, init, destroy)			\
  resource_list_destroy (id, &rm->nursery_list(id));	\
  resource_list_destroy (id, &rm->heap_list(id));	\
  resource_list_destroy (id, &rm->free_list(id));
  RESOURCES
#undef ENTRY
}

#define ENTRY(id, type, init, destroy)			\
  Resource(id) *					\
  resource_manager_allocate_##id (ResourceManager *rm)	\
  {							\
    Resource(id) *res = deque_pop (&rm->free_list(id));	\
    if (res == NULL)				        \
    {                                                   \
      res = XMALLOC (Resource(id));                     \
      res->header = TYPE(id);				\
      init (res->payload);				\
    }                                                   \
    res->in_nursery = true;                             \
    deque_insert (&rm->nursery_list(id), res);          \
    return res;                                         \
  }
RESOURCES
#undef ENTRY

void
resource_manager_begin_gc (ResourceManager *rm, bool major_gc)
{
  rm->major_gc = major_gc;
  
  if (!major_gc)
    return;

#define ENTRY(id, type, init, destroy)				\
  deque_concat (&rm->nursery_list(id), &rm->heap_list(id));
  RESOURCES
#undef ENTRY
}

void
resource_manager_end_gc (ResourceManager *rm)
{
#define ENTRY(id, type, init, destroy)				\
  deque_concat (&rm->free_list(id), &rm->nursery_list(id));
  RESOURCES
#undef ENTRY
}

#define ENTRY(id, type, init, destroy)					\
  void									\
  resource_manager_mark_##id (ResourceManager *rm, Resource(id) *res)	\
  {									\
    if (!rm->major_gc && !res->in_nursery)				\
      return;								\
    res->in_nursery = false;						\
    deque_remove (&rm->nursery_list(id), res);				\
    deque_insert (&rm->heap_list(id), res);				\
  }
RESOURCES
#undef ENTRY

