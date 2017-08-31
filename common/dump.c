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
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitrotate.h"
#include "gl_linkedhash_list.h"
#include "gl_xlist.h"
#include "obstack.h"
#include "unistr.h"
#include "unistdio.h"
#include "vmcommon.h"
#include "xmalloca.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

static bool
has_location (Object obj)
{
  // TODO (XXX): Records, ports, etc.
  return is_string (obj)
    || is_pair (obj)
    || is_vector (obj)
    || is_closure (obj)
    || is_procedure (obj);
}

static void
dump_raw_string (uint32_t const *s, size_t n, FILE *out)
{
  for (size_t i = 0; i < n; ++i)
    {
      if (i != 0)
	fputc (' ', out);
      write_char (s[i], out);
    }
}

static void
dump_unwritable_symbol (uint8_t const *s, size_t n, FILE *out)
{
  size_t len;
  uint32_t *s32 = u8_to_u32 (s, n, NULL, &len);

  fputs ("(symbol ", out);
  dump_raw_string (s32, len, out);
  fputc (')', out);
  
  free (s32);
}

static void
dump_symbol (Object obj, FILE *out)
{
  uint8_t *s = symbol_bytes (obj);
  size_t len = symbol_length (obj);
  if (!parse_symbol (s, len))
    {
      dump_unwritable_symbol (s, len, out);
    }
  else
    {
      fputc ('\'', out);
      scheme_write (obj, out);
    }
}

static void
dump_string (Object obj, FILE *out)
{
  uint32_t *s = string_bytes (obj);
  size_t n = string_length (obj);
  if (u32_chr (s, n, 0x0a))
    {
      fputs ("(string ", out);
      dump_raw_string (s, n, out);
      fputc (')', out);
    }
  else
    {
      fputc ('"', out);
      for (size_t i = 0; i < n; ++i)
	switch (s[i])
	  {
	  case 0x22:
	    fputs ("\\\"", out);
	    break;
	  case 0x5c:
	    fputs ("\\\\", out);
	    break;
	  default:
	    {
	      uint32_t b[2] = { s[i], 0 };
	      ulc_fprintf (out, "%llU", b);
	    }
	  }
      fputc ('"', out);
    }
}

static void
dump_procedure (Object proc, FILE *out)
{
  fputs ("(code '", out);
  /* FIXME(XXX): Not every object in code may be printable. */
  scheme_write (procedure_code (proc), out);
  fputc (')', out);
}

struct setter
{
  Object obj;
  size_t index;
};

void
add_setter (struct obstack *setters, Object obj, size_t index)
{
  obstack_grow (setters, &((struct setter) { .obj = obj, .index = index }),
		sizeof (struct setter));
};
  
struct entry
{
  Object obj;
  long int id;
  bool defined;
};

static bool
comparator (void const *entry1, void const *entry2)
{
  return ((struct entry const *) entry1)->obj
    == ((struct entry const *) entry2)->obj;
}

static size_t
hasher (void const *entry)
{
  return rotr_sz ((size_t) ((struct entry const *) entry)->obj, 3);
}

static struct entry *
lookup (gl_list_t obj_table, Object obj)
{
  gl_list_node_t node = gl_list_search (obj_table, &(struct entry) { .obj = obj });
  if (node == NULL)
    return NULL;
  return (struct entry *) gl_list_node_value (obj_table, node);
}

static bool
insert (gl_list_t obj_table, Object obj, long int *count)
{
  if (lookup (obj_table, obj))
    return true;
  struct entry *restrict entry = XMALLOC (struct entry);
  entry->obj = obj;
  entry->id = (*count)++;
  entry->defined = false;
  gl_list_add_first (obj_table, entry);

  return false;
}

static void
scan_object (Object obj, gl_list_t obj_table)
{
  struct frame
  {
    Object obj;
    ptrdiff_t index;
    struct frame *prev;
  };

  struct frame *top = NULL;
  long int count = 0;

  do
    {
      if (has_location (obj))
	{
	  if (!insert (obj_table, obj, &count)
	      && (is_pair (obj) || is_vector (obj) || is_closure (obj)))
	    {
	      struct frame *frame = xmalloca (sizeof (struct frame));
	      frame->obj = obj;
	      frame->index = is_closure (obj) ? -1 : 0;
	      frame->prev = top;
	      top = frame;
	    }
	}

      while (top != NULL)
	{
	  struct frame *frame = top;
	  if (is_pair (frame->obj))
	    {
	      /* Pair */
	      switch (frame->index)
		{
		case 0:
		  obj = car (frame->obj);
		  break;
		case 1:
		  obj = cdr (frame->obj);
		  break;
		case 2:
		  top = frame->prev;
		  freea (frame);
		  continue; /* while */
		}
	      frame->index++;
	      break; /* while */
	    }
	  else if (is_vector (frame->obj))
	    /* Vector */
	    {
	      if (frame->index == vector_length (frame->obj))
		{
		  top = frame->prev;
		  freea (frame);
		  continue; /* while */
		}
	      obj = vector_ref (frame->obj, frame->index++);
	      break; /* while */
	    }
	  else
	    /* Closure */
	    {
	      if (frame->index == closure_length (frame->obj))
		{
		  top = frame->prev;
		  freea (frame);
		  continue; /* while */
		}
	      if (frame->index == -1)
		obj = closure_procedure (frame->obj);
	      else
		obj = closure_ref (frame->obj, frame->index);
	      frame->index++;
	      break;
	    }
	}
    }
  while (top != NULL);
}

static void
dump_object (Object obj, gl_list_t obj_table, struct obstack *setters, FILE *out)
{  
  struct frame
  {
    Object obj;
    ptrdiff_t index;
    struct frame *prev;
  };

  struct frame *top = NULL;

  do
    {
      struct entry *entry = lookup (obj_table, obj);
      if (entry != NULL && (top != NULL || entry->defined) && entry->id >= 0)
	{      
	  if (!entry->defined)
	    {
	      fputs ("#f", out);
	      add_setter (setters, top->obj, top->index - 1);
	    }
	  else
	    fprintf (out, "$%ld", entry->id);
	}
      else if (is_pair (obj))
	{
	  fputs ("(cons ", out);
	  struct frame *frame = xmalloca (sizeof (struct frame));
	  frame->obj = obj;
	  frame->index = 0;
	  frame->prev = top;
	  top = frame;
	}
      else if (is_vector (obj))
	{
	  fputs ("(vector ", out);
	  struct frame *frame = xmalloca (sizeof (struct frame));
	  frame->obj = obj;
	  frame->index = 0;
	  frame->prev = top;
	  top = frame;
	}
      else if (is_closure (obj))
	{
	  fputs ("(closure", out);
	  struct frame *frame = xmalloca (sizeof (struct frame));
	  frame->obj = obj;
	  frame->index = -1;
	  frame->prev = top;
	  top = frame;
	}
      else if (is_string (obj))
	dump_string (obj, out);
      else if (is_null (obj))
	fputs ("'()", out);
      else if (is_eof_object (obj))
	fputs ("eof-object)", out);
      else if (is_symbol (obj))
	dump_symbol (obj, out);
      else if (is_procedure (obj))
	dump_procedure (obj, out);
      else
	/* The object is self-evaluating. */
	scheme_write (obj, out);

      while (top != NULL)
	{
	  struct frame *frame = top;
	  if (is_pair (frame->obj))
	    /* Pair */
	    {
	      switch (frame->index)
		{
		case 0:
		  obj = car (frame->obj);
		  frame->index++;
		  break;
		case 1:
		  fputc (' ', out);
		  obj = cdr (frame->obj);
		  frame->index++;
		  break;
		case 2:
		  fputc (')', out);
		  top = frame->prev;
		  freea (frame);
		  continue; /* while */
		}
	      break; /* while */
	    }
	  else if (is_vector (frame->obj))
	    /* Vector */
	    {
	      if (frame->index == vector_length (frame->obj))
		{
		  fputc (')', out);
		  top = frame->prev;
		  freea (frame);
		  continue; /* while */
		}
	      if (frame->index > 0)
		fputc (' ', out);
	      obj = vector_ref (frame->obj, frame->index++);
	      break; /* while */
	    }
	  else
	    /* Closure */
	    {
	      if (frame->index == closure_length (frame->obj))
		{
		  fputc (')', out);
		  top = frame->prev;
		  freea (frame);
		  continue; /* while */
		}
	      fputc (' ', out);
	      obj = frame->index < 0
		? closure_procedure (frame->obj)
		: closure_ref (frame->obj, frame->index);
	      ++frame->index;
	      break; /* while */
	    }
	}

    }
  while (top != NULL);
}

static void
dump_definition (struct entry *entry, gl_list_t obj_table, struct obstack *setters, FILE *out)
{
  fprintf (out, "(define $%ld ", entry->id);
  dump_object (entry->obj, obj_table, setters, out);
  fputs (")\n", out);
  entry->defined = true;
}
static void
dump_definitions (gl_list_t obj_table, struct obstack *setters, FILE *out)
{
  gl_list_iterator_t iter = gl_list_iterator (obj_table);
  struct entry *entry;
  while (gl_list_iterator_next (&iter, (void const **) &entry, NULL))
    if (is_string (entry->obj) || is_procedure (entry->obj))
      dump_definition (entry, obj_table, setters, out);
  gl_list_iterator_free (&iter);
  iter = gl_list_iterator (obj_table);
  while (gl_list_iterator_next (&iter, (void const **) &entry, NULL))
    if (!is_string (entry->obj) && !is_procedure (entry->obj))
      dump_definition (entry, obj_table, setters, out);
  gl_list_iterator_free (&iter);
}

static void
dump_setters (gl_list_t obj_table, struct obstack *setters, FILE *out)
{
  struct setter *const end = obstack_next_free (setters);
  for (struct setter *restrict setter = (struct setter *) (obstack_base (setters));
       setter < end;
       ++setter)
    {
      struct entry *entry = lookup (obj_table, setter->obj);
      if (is_pair (setter->obj))
	fprintf (out,
		 "(set-%s! $%ld $%ld)\n",
		 setter->index == 0 ? "car" : "cdr",
		 entry->id,
		 lookup (obj_table,
			 (setter->index == 0 ? car (setter->obj) : cdr (setter->obj)))->id);
      else if (is_vector (setter->obj))
	fprintf (out,
		 "(vector-set! $%ld %zd $%ld)\n",
		 entry->id,
		 setter->index,
		 lookup (obj_table, (vector_ref (setter->obj, setter->index)))->id);
      else /* closure */
	fprintf (out,
		 "(closure-set! $%ld %zd $%ld)\n",
		 entry->id,
		 setter->index,
		 lookup (obj_table, (closure_ref (setter->obj, setter->index)))->id);
    }  
}

void
dump (Object obj, FILE *out)
{
  gl_list_t obj_table
    = gl_list_create_empty (GL_LINKEDHASH_LIST, comparator, hasher,
			    (void (*) (void const *)) free, false);

  struct obstack setters;
  obstack_init (&setters);
  
  scan_object (obj, obj_table);

  dump_definitions (obj_table, &setters, out);

  dump_setters (obj_table, &setters, out);

  dump_object (obj, obj_table, NULL, out);
  fputc ('\n', out);
  
  obstack_finish (&setters);
  obstack_free (&setters, NULL);

  gl_list_free (obj_table);
}
