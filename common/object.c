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
#include <stddef.h>

#include "error.h"
#include "uniconv.h"
#include "unistr.h"
#include "unitypes.h"

#include "vmcommon.h"

bool
is_immediate (Object object)
{
  return (object & OBJECT_TYPE_MASK) == IMMEDIATE_TYPE;
}

bool
is_pointer (Object object)
{
  return !is_immediate (object);
}

bool
is_unmanaged (Pointer pointer)
{
  if (is_pair ((Object) pointer))
    return false;
  return (pointer[-1] & (OBJECT_TYPE_MASK | UNMANAGED_TYPE))
    == (HEADER_TYPE | UNMANAGED_TYPE);
}

Object
header_type (Pointer pointer)
{
  return pointer[-1] & HEADER_TYPE_MASK;
}

bool
is_binary (Object header)
{
  return (header & BINARY_TYPE) == BINARY_TYPE;
}

bool
is_well_known_symbol (Pointer pointer)
{
  if (is_pair ((Object) pointer))
    return false;
  return (pointer[-1] & WELL_KNOWN_SYMBOL) == WELL_KNOWN_SYMBOL;
}

static bool
is_marked (Object object)
{
  return (object & OBJECT_TYPE_MASK) == MARKED_TYPE;
}

static bool
is_link (Object object)
{
  return (object & OBJECT_TYPE_MASK) == LINK_TYPE;
}

static size_t
object_payload (Object object)
{
  return object & ~OBJECT_TYPE_MASK;
}

Pointer
forwarding_address (Object from_header)
{
  return is_marked (from_header) ? (Pointer) object_payload (from_header) : NULL;
}

static Object
align_size (Object size)
{
  return (size + 2 * WORDSIZE - 1) & ~(2 * WORDSIZE - 1);
}

static bool
is_header (Object object)
{
  return (object & OBJECT_TYPE_MASK) == HEADER_TYPE;
}

static Object
header_payload (Object header)
{
  return header >> HEADER_SIZE_SHIFT;
}

Pointer
object_header (Pointer pointer)
{
  if (is_pair ((Object) pointer))
    return pointer;

  pointer--;
  
  if (is_header (*pointer) || is_marked (*pointer))
    return pointer;

  /* If it is not a header, it has to be a link field. */
  return pointer - (object_payload (*pointer) / WORDSIZE);
}

size_t
object_size (Pointer pointer)
{
  if (!is_header (*pointer))
    /* If there is no header, the object has to be a pair. */
    return 2;

  size_t payload = header_payload (*pointer);
  if (!payload)
    /* If the size is not encoded in the header, it is a large object,
       and the size (minus two header words) follows in the next
       word. */
    return align_size (pointer [1]) / WORDSIZE + 2;

  return align_size (payload) / WORDSIZE;
}

Object
make_mark (Pointer to)
{
  return (Object) to | MARKED_TYPE;
}

Pointer
object_pointers (Pointer header)
{
  if (is_binary (*header))
    return NULL;

  Pointer end = header + object_size (header);
  
  /* Skip size field if present. */  
  Pointer start = header_payload (*header) ? header + 2 : header + 1;
  
  while (start < end)
    {
      if (is_link (*start))
	{
	  start += 2;
	  continue;
	}
      break;
    }

  return start;
}

Object
make_undefined ()
{
  return UNDEFINED_TYPE;
}

bool
is_undefined (Object object)
{
  return (object & IMMEDIATE_TYPE_MASK) == UNDEFINED_TYPE;
}

void
check_defined (Object obj)
{
  if (is_undefined (obj))
    error (EXIT_FAILURE, 0, "attempt to reference undefined variable");
}

Object
make_null ()
{
  return NULL_TYPE;
}

bool
is_null (Object object)
{
  return (object & IMMEDIATE_TYPE_MASK) == NULL_TYPE;
}

Object
make_eof_object ()
{
  return EOF_TYPE;
}

bool
is_eof_object (Object object)
{
  return (object & IMMEDIATE_TYPE_MASK) == EOF_TYPE;
}

Object
make_char (uint32_t c)
{
  return (c << IMMEDIATE_PAYLOAD_SHIFT) | CHAR_TYPE;  
}

uint32_t
char_value (Object c)
{
  return c >> IMMEDIATE_PAYLOAD_SHIFT;
}

bool
is_char (Object object)
{
  return (object & IMMEDIATE_TYPE_MASK) == CHAR_TYPE;
}

Object
make_boolean (bool v)
{
  return ((v ? true : false) << IMMEDIATE_PAYLOAD_SHIFT) | BOOLEAN_TYPE;
}

bool
boolean_value (Object b)
{
  return b >> IMMEDIATE_PAYLOAD_SHIFT;
}

bool
is_boolean (Object obj)
{
  return (obj & IMMEDIATE_TYPE_MASK) == BOOLEAN_TYPE;
}

Object
cons (Heap *heap, Object car, Object cdr)
{
  object_stack_grow (&heap->stack, car);
  object_stack_grow (&heap->stack, cdr);
  return object_stack_finish (&heap->stack) | PAIR_TYPE;
}

Object
car (Object pair)
{
  return ((Pointer) pair)[0];
}

Object
cdr (Object pair)
{
  return ((Pointer) pair)[1];
}

Object
cadr (Object pair)
{
  return car (cdr (pair));
}

Object
cddr (Object pair)
{
  return cdr (cdr (pair));
}

void
set_car (Heap *heap, Object pair, Object car)
{
  mutate (heap, ((Pointer) pair), car);
}

void
set_cdr (Heap *heap, Object pair, Object cdr)
{
  mutate (heap, ((Pointer) pair) + 1, cdr);
}

bool
is_pair (Object object)
{
  return (object & OBJECT_TYPE_MASK) == PAIR_TYPE;
}

Object
make_string (Heap *heap, size_t length, ucs4_t c)
{
  object_stack_grow (&heap->stack, STRING_TYPE);
  object_stack_grow (&heap->stack, (length + 1) * sizeof (ucs4_t));
  for (int i = 0; i < length; ++i)
    object_stack_ucs4_grow (&heap->stack, c);
  object_stack_ucs4_grow (&heap->stack, 0);
  object_stack_align (&heap->stack);
  return object_stack_finish (&heap->stack) | POINTER_TYPE;
}

uint32_t *
string_bytes (Object string)
{
  return (uint32_t *) (((Pointer) string) + 1);
}

size_t
string_length (Object string)
{
  return ((Pointer) string)[0] / sizeof (ucs4_t) - 1;
}

char *
string_value (Object sym)
{
  void *s = u32_strconv_to_locale ((uint32_t const *) &((Pointer) sym)[1]);
  if (s == NULL)
    xalloc_die ();
  return s;
}

ucs4_t
string_ref (Object string, size_t index)
{
  return ((uint32_t *) (((Pointer) string) + 1))[index];
}

void
string_set (Object string, size_t index, ucs4_t c)
{
  ((uint32_t *) (((Pointer) string) + 1))[index] = c;
}

bool
is_string (Object object)
{
  return (object & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) object)[-1] & HEADER_TYPE_MASK) == STRING_TYPE;
}

Object
string (Heap *heap, Object chars)
{
  int n;
  object_stack_grow (&heap->stack, STRING_TYPE);
  object_stack_grow (&heap->stack, 0);
  for (n = 0; !is_null (chars); chars = cdr (chars), ++n)
    object_stack_ucs4_grow (&heap->stack, char_value (car (chars)));
  object_stack_align (&heap->stack);
  Object str = object_stack_finish (&heap->stack) | POINTER_TYPE;
  ((Pointer) str)[0] = n * sizeof (ucs4_t);
  return str;  
}

Object
make_symbol (Heap *heap, uint8_t *s, size_t len)
{
  object_stack_grow (&heap->stack, SYMBOL_TYPE);
  object_stack_grow (&heap->stack, (len + 1) * sizeof (uint8_t));
  object_stack_utf8_grow (&heap->stack, s, len);
  object_stack_grow0 (&heap->stack);
  object_stack_align (&heap->stack);
  Object sym = object_stack_finish (&heap->stack) | POINTER_TYPE;
  return symbol_table_intern (&heap->symbol_table, sym, false);
}

bool
is_symbol (Object object)
{
  return (object & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) object)[-1] & HEADER_TYPE_MASK) == SYMBOL_TYPE;
}

uint8_t *
symbol_bytes (Object sym)
{
  return (uint8_t *) (((Pointer) sym) + 1);
}

size_t
symbol_length (Object sym)
{
  return ((Pointer) sym)[0] / sizeof (uint8_t) - 1;
}

char *
symbol_value (Object sym)
{
  void *s = u8_strconv_to_locale ((uint8_t const *) &((Pointer) sym)[1]);
  if (s == NULL)
    xalloc_die ();
  return s;
}

Object
symbol (Heap *heap, Object chars)
{
  int n;
  uint8_t s[6];
  object_stack_grow (&heap->stack, SYMBOL_TYPE);
  object_stack_grow (&heap->stack, 0);
  for (n = 0; !is_null (chars); chars = cdr (chars), ++n)
    {
      size_t len = u8_uctomb (s, char_value (car (chars)), 6);
      object_stack_utf8_grow (&heap->stack, s, len);
    }
  object_stack_grow0 (&heap->stack);
  object_stack_align (&heap->stack);
  Object sym = object_stack_finish (&heap->stack) | POINTER_TYPE;
  ((Pointer) sym)[0] = n * sizeof (uint8_t);
  return symbol_table_intern (&heap->symbol_table, sym, false);
}

Object
make_vector (Heap *heap, size_t length, Object object)
{
  object_stack_grow (&heap->stack, VECTOR_TYPE);
  object_stack_grow (&heap->stack, WORDSIZE * length);
  for (int i = 0; i < length; ++i)
    object_stack_grow (&heap->stack, object);
  object_stack_align (&heap->stack);
  return object_stack_finish (&heap->stack) | POINTER_TYPE;
}

size_t
vector_length (Object vector)
{
  return ((Pointer) vector)[0] / WORDSIZE;
}

Object
vector_ref (Object vector, size_t index)
{
  return ((Pointer) vector)[index + 1];
}

void
vector_set (Heap *heap, Object vector, size_t index, Object value)
{
  mutate (heap, ((Pointer) vector) + index + 1, value);
}

bool
is_vector (Object object)
{
  return (object & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) object)[-1] & HEADER_TYPE_MASK) == VECTOR_TYPE;
}

Object
make_procedure (Heap *heap, Object code)
{
  object_stack_grow (&heap->stack, PROCEDURE_TYPE);
  object_stack_grow (&heap->stack, compile (heap, code));
  object_stack_grow (&heap->stack, code);
  object_stack_align (&heap->stack);
  return object_stack_finish (&heap->stack) | POINTER_TYPE;
}

/* TODO (XXX): Lift the assembly functions to the procedure level. */
Object
procedure_assembly (Object proc)
{
  return ((Pointer) proc)[0];
}

bool
is_procedure (Object obj)
{
  return (obj & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) obj)[-1] & HEADER_TYPE_MASK) == (PROCEDURE_TYPE & HEADER_TYPE_MASK);
}

Object
make_closure (Heap *heap, Object proc, size_t slots, Object obj)
{
  Object assembly = procedure_assembly (proc);
  size_t entries = assembly_entry_point_number (assembly);
  EntryPoint *entry_points = assembly_entry_points (assembly);
  
  object_stack_grow (&heap->stack, CLOSURE_TYPE);
  object_stack_grow (&heap->stack, (1 + slots + 2 * entries) * WORDSIZE);
  size_t offset = 2;
  for (int i = 0; i < entries; ++i)
    {
      object_stack_grow (&heap->stack, offset * WORDSIZE | LINK_TYPE);
      object_stack_grow (&heap->stack, (Object) entry_points [i]);
      offset += 2;
    }
  for (int i = 0; i < slots; ++i)
    object_stack_grow (&heap->stack, obj);
  object_stack_grow (&heap->stack, proc);
  object_stack_align (&heap->stack);
  return object_stack_finish (&heap->stack) | POINTER_TYPE;
}

Object
closure_procedure (Object closure)
{
  size_t size = ((Pointer) closure) [0] / WORDSIZE;
  return ((Pointer) closure)[size];
}

Object
closure_ref (Object closure, size_t index)
{
  size_t size = ((Pointer) closure) [0] / WORDSIZE;
  return ((Pointer) closure)[size - index - 1];
}

Object
closure_set (Heap *heap, Object closure, size_t index, Object val)
{
  size_t size = ((Pointer) closure) [0] / WORDSIZE;
  mutate (heap, ((Pointer) closure) + size - index - 1, val);  
}

bool
is_closure (Object obj)
{
  return (obj && OBJECT_TYPE_MASK) == POINTER_TYPE
    &&(((Pointer) obj)[-1] & HEADER_TYPE_MASK) == CLOSURE_TYPE;
}


/* Exact numbers */

/* The value in Q is destroyed after calling this function. */
Object
make_exact_number (Heap *restrict heap, mpq_t q)
{
  Resource(EXACT_NUMBER) *res
    = resource_manager_allocate (EXACT_NUMBER, &heap->resource_manager);
  mpq_swap (RESOURCE_PAYLOAD (res), q);
  return (Object) res | POINTER_TYPE;
}

bool
is_exact_number (Object object)
{
  return (object & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) object)[-1] & HEADER_TYPE_MASK) == EXACT_NUMBER_TYPE;
}
  
mpq_t *
exact_number_value (Object num)
{
  return (mpq_t *) num;
}

/* The value in X is destroyed after calling this function. */
Object
make_inexact_number (Heap *restrict heap, mpc_t x)
{
  Resource(INEXACT_NUMBER) *res
    = resource_manager_allocate (INEXACT_NUMBER, &heap->resource_manager);
  mpc_swap (RESOURCE_PAYLOAD (res), x);
  return (Object) res | POINTER_TYPE;
}

bool
is_inexact_number (Object object)
{
  return (object & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) object)[-1] & HEADER_TYPE_MASK) == INEXACT_NUMBER_TYPE;
}

mpc_t *
inexact_number_value (Object num)
{
  return (mpc_t *) num;
}
