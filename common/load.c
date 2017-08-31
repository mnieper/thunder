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
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitrotate.h"
#include "error.h"
#include "hash.h"
#include "obstack.h"
#include "vmcommon.h"
#include "xmalloca.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

struct entry
{
  Object identifier;
  Object obj;
};

static bool
comparator (void const *entry1, void const *entry2)
{
  return ((struct entry const *) entry1)->identifier
    == ((struct entry const *) entry2)->identifier;
}

static size_t
hasher (void const *entry, size_t n)
{
  size_t val = rotr_sz ((size_t) ((struct entry const *) entry)->identifier, 3);
  return val % n;
}

static struct entry *
lookup (Hash_table *obj_table, Object identifier)
{
  return (struct entry *)
    hash_lookup (obj_table,
		 &(struct entry) { .identifier = identifier });
}

static struct entry *
insert (Hash_table *obj_table, Object id)
{
  struct entry *entry = XMALLOC (struct entry);
  void *res;
  entry->identifier = id;
  if ((res = hash_insert (obj_table, entry)) != entry)
    {
      if (res == NULL)
	xalloc_die ();
      free (entry);
      return NULL;
    }
  return entry;
}

Object
load_object (Heap *heap, Hash_table *obj_table, Object expr, Location *loc, char const *filename)
{
  enum frame_type { FRAME_PAIR, FRAME_VECTOR, FRAME_CLOSURE };
  
  struct frame
  {
    enum frame_type type;
    Object obj;
    Object expr;
    ptrdiff_t index;
    struct frame *prev;
  };
  
  struct frame *top = NULL;
  
  do
    {
      if (is_list (expr))
	{
	  Object op = car (expr);
	  if (op == SYMBOL(QUOTE))
	    expr = cadr (expr);
	  else if (op == SYMBOL(STRING))
	    expr = string (heap, cdr (expr));
	  else if (op == SYMBOL(SYMBOL))
	    expr = symbol (heap, cdr (expr));
	  else if (op == SYMBOL(CODE))
	    expr = make_procedure (heap, cdr (expr));
	  else if (op == SYMBOL(CONS))
	    {
	      struct frame *frame = xmalloca (sizeof (struct frame));
	      frame->type = FRAME_PAIR;
	      frame->obj = cons (heap, make_undefined (), make_undefined ());
	      frame->expr = cdr (expr);
	      frame->index = 0;
	      frame->prev = top;
	      top = frame;
	    }
	  else if (op == SYMBOL(VECTOR))
	    {
	      struct frame *frame = xmalloca (sizeof (struct frame));
	      frame->type = FRAME_VECTOR;
	      frame->expr = cdr (expr);
	      frame->obj = make_vector (heap, length (frame->expr), make_undefined ());
	      frame->index = 0;
	      frame->prev = top;
	      top = frame;	      
	    }
	  else if (op == SYMBOL(CLOSURE))
	    {
	      struct frame *frame = xmalloca (sizeof (struct frame));
	      frame->type = FRAME_CLOSURE;
	      frame->expr = cdr (expr);
	      frame->index = -1;
	      frame->prev = top;
	      top = frame;
	    }
	  else
	    error_at_line (EXIT_FAILURE, 0, filename, loc->first_line,
			   "invalid keyword: %s", object_get_str (op));
	}
      else if (is_pair (expr))
	error_at_line (EXIT_FAILURE, 0, filename, loc->first_line,
		       "dotted pair in source: %s", object_get_str (expr));
      else if (is_symbol (expr))
	{
	  struct entry *entry = lookup (obj_table, expr);
	  if (entry == NULL)
	    error_at_line (EXIT_FAILURE, 0, filename, loc->first_line,
			   "undefined variable: %s", object_get_str (expr));
	  expr = entry->obj;
	}

      /* TODO: Simplify logic. */
      while (top != NULL)
	{
	  if (top->index > 0)
	    {
	      switch (top->type)
		{
		case FRAME_PAIR:
		  if (top->index == 1)
		    set_car (heap, top->obj, expr);
		  else
		    set_cdr (heap, top->obj, expr);
		  break;
		case FRAME_VECTOR:
		  vector_set (heap, top->obj, top->index - 1, expr);
		  break;
		case FRAME_CLOSURE:
		  closure_set (heap, top->obj, top->index - 1, expr);
		  break;
		}
	    }
	  else if (top->index == 0 && top->type == FRAME_CLOSURE)
	    {
	      assert_procedure (expr);
	      top->obj = make_closure (heap, expr, length (top->expr), make_undefined ());
	    }
	  
	  struct frame *frame = top;
	  if (is_null (frame->expr))
	    {
	      expr = frame->obj;
	      top = frame->prev;
	      freea (frame->prev);
	      continue;
	    }

	  /* TODO (XXX): The three cases seem to be alike. */
	  if (frame->type == FRAME_PAIR)
	    {
	      expr = car (frame->expr);
	      frame->expr = cdr (frame->expr);
	      frame->index++;
	      break;
	    }
	  else if (frame->type == FRAME_VECTOR)
	    /* Vector */
	    {
	      expr = car (frame->expr);
	      frame->expr = cdr (frame->expr);
	      frame->index++;
	      break;
	    }
	  else
	    /* Closure */
	    {
	      expr = car (frame->expr);
	      frame->expr = cdr (frame->expr);
	      frame->index++;
	      break;
	    }
	}
      
    }
  while (top != NULL);
  
  return expr;
}

Object
checked_read (Reader *reader, Location *loc, char const *filename)
{
  Object expr;
  char const *msg;
  
  if (!(reader_read (reader, &expr, loc, &msg)))
    error_at_line (EXIT_FAILURE, 0, filename, loc->first_line, "%s", msg);
  
  return expr;
}

Object
load (Heap *heap, FILE *in, char const *filename)
{
  Hash_table *obj_table = hash_initialize (0, NULL, hasher, comparator, free);
  if (obj_table == NULL)
    xalloc_die ();
  
  Object expr;
  
  Reader reader;
  reader_init (&reader, heap, in);
  Location loc;

  while (!is_eof_object (expr = checked_read (&reader, &loc, filename)))
    {
      if (is_pair (expr))
	{
	  if (!is_list (cdr (expr)))
	    error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			   "dotted pair in source: %s", object_get_str (expr));
	  
	  Object op = car (expr);
	  
	  if (op == SYMBOL(DEFINE))
	    {
	      Object id = cadr (expr);
	      if (!is_symbol (id))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "can't define a non-symbol: %s", object_get_str (id));

	      struct entry *entry = insert (obj_table, id);
	      if (entry == NULL)
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "multiple definition for: %s", object_get_str (id));
	      
	      entry->obj = load_object (heap, obj_table, car (cddr (expr)), &loc, filename);

	      continue;
	    }

	  /* XXX: Too much code duplicated. */
	  if (op == SYMBOL(SET_CAR))
	    {
	      Object id = cadr (expr);
	      if (!is_symbol (id))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "can't set a non-variable: %s", object_get_str (id));
	      
	      struct entry *entry = lookup (obj_table, id);
	      if (entry == NULL)
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "undefined variable: %s", object_get_str (id));

	      if (!is_pair (entry->obj))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "not a pair: %s", object_get_str (id));

	      set_car (heap, entry->obj, load_object (heap, obj_table, car (cddr (expr)),
						      &loc, filename));
	      
	      continue;
	    }

	  if (op == SYMBOL(SET_CDR))
	    {
	      Object id = cadr (expr);
	      if (!is_symbol (id))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "can't set a non-variable: %s", object_get_str (id));
	      
	      struct entry *entry = lookup (obj_table, id);
	      if (entry == NULL)
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "undefined variable: %s", object_get_str (id));

	      if (!is_pair (entry->obj))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "not a pair: %s", object_get_str (id));

	      set_cdr (heap, entry->obj, load_object (heap, obj_table, car (cddr (expr)),
						      &loc, filename));
	      
	      continue;
	    }
	      
	  if (op == SYMBOL(VECTOR_SET))
	    {
	      Object id = cadr (expr);
	      if (!is_symbol (id))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "can't set a non-variable: %s", object_get_str (id));
	      
	      struct entry *entry = lookup (obj_table, id);
	      if (entry == NULL)
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "undefined variable: %s", object_get_str (id));

	      if (!is_vector (entry->obj))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "not a vector: %s", object_get_str (id));

	      Object index = car (cddr (expr));

	      if (!is_exact_number (index))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "not an exact number: %s", object_get_str (index));
	      
	      vector_set (heap, entry->obj, fixnum (index),
			  load_object (heap, obj_table, cadr (cddr (expr)),
				       &loc, filename));
	      
	      continue;
	    }

	  if (op == SYMBOL(CLOSURE_SET))
	    {
	      Object id = cadr (expr);
	      if (!is_symbol (id))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "can't set a non-variable: %s", object_get_str (id));
	      
	      struct entry *entry = lookup (obj_table, id);
	      if (entry == NULL)
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "undefined variable: %s", object_get_str (id));

	      if (!is_closure (entry->obj))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "not a closure: %s", object_get_str (id));

	      Object index = car (cddr (expr));

	      if (!is_exact_number (index))
		error_at_line (EXIT_FAILURE, 0, filename, loc.first_line,
			       "not an exact number: %s", object_get_str (index));
	      
	      closure_set (heap, entry->obj, fixnum (index),
			   load_object (heap, obj_table, cadr (cddr (expr)),
					&loc, filename));
	      
	      continue;
	    }
	}
      
      expr = load_object (heap, obj_table, expr, &loc, filename);
      break;
    }
  
  reader_destroy (&reader);

  hash_free (obj_table);

  return expr;
}
