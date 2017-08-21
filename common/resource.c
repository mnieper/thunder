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

// XXX X-Macro for different resource types

static void
resource_list_destroy (ResourceList *deque)
{
  Resource *res;
  while ((res = deque_pop (deque)) != NULL)
    {
      mpq_clear (res->number);
      free (res);
    }
}

void
resource_manager_init (ResourceManager *rm)
{
  deque_init (&rm->nursery_list);
  deque_init (&rm->heap_list);
  deque_init (&rm->free_list);
}

void
resource_manager_destroy (ResourceManager *rm)
{
  resource_list_destroy (&rm->nursery_list);
  resource_list_destroy (&rm->heap_list);
  resource_list_destroy (&rm->free_list);
}

Resource *
resource_manager_allocate (ResourceManager *rm)
{
  Resource *res = deque_pop (&rm->free_list);
  
  if (res == NULL)
    {
      res = XMALLOC (Resource);
      res->header = EXACT_NUMBER_TYPE;
      mpq_init (res->number);
    }

  res->in_nursery = true;

  deque_insert (&rm->nursery_list, res);

  return res;
}

void
resource_manager_begin_gc (ResourceManager *rm, bool major_gc)
{
  rm->major_gc = major_gc;
  
  if (!major_gc)
    return;

  deque_concat (&rm->nursery_list, &rm->heap_list);
}

void
resource_manager_end_gc (ResourceManager *rm)
{
  deque_concat (&rm->free_list, &rm->nursery_list);
}

void
resource_manager_mark (ResourceManager *rm, Resource *res)
{
  if (!rm->major_gc && !res->in_nursery)
    return;

  res->in_nursery = false;
  
  deque_remove (&rm->nursery_list, res);
  deque_insert (&rm->heap_list, res);
}
