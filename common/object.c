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
  return pointer - object_payload (*pointer) / WORDSIZE;
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

  size_t size = object_size (header);
  
  /* Skip size field if present. */  
  Pointer start;
  if (header_payload (*header))
    {
      start = header + 2;
      size -= 2;
    }
  else
    {
      start = header + 1;
      size -= 1;
    }

  while (size > 0 && is_link (*header))
    {
      size -= 2;
      start += 2;
    }
  
  return start;
}

static void
stack_grow (Heap *heap, Object object)
{
  obstack_grow (&heap->stack, &object, sizeof (object));
}

static void
stack_ucs4_grow (Heap *heap, ucs4_t c)
{
  obstack_grow (&heap->stack, &c, sizeof (c));
}

static void
stack_utf8_grow (Heap *heap, uint8_t *s, size_t len)
{
  obstack_grow (&heap->stack, s, len * sizeof (uint8_t));
}

static void
stack_align (Heap *heap)
{
  size_t size = obstack_object_size (&heap->stack);
  
  obstack_blank (&heap->stack, ((size + ALIGNMENT_MASK) & ~ALIGNMENT_MASK) - size);
}

static Object
stack_finish (Heap *heap)
{
  return (Object) obstack_finish (&heap->stack);
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
  stack_grow (heap, car);
  stack_grow (heap, cdr);
  return stack_finish (heap) | PAIR_TYPE;
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
set_car (Object pair, Object car)
{
  ((Pointer) pair)[0] = car;
}

void
set_cdr (Object pair, Object cdr)
{
  ((Pointer) pair)[1] = cdr;
}

bool
is_pair (Object object)
{
  return (object & OBJECT_TYPE_MASK) == PAIR_TYPE;
}

Object
make_string (Heap *heap, size_t length, ucs4_t c)
{
  stack_grow (heap, STRING_TYPE);
  stack_grow (heap, length * sizeof (ucs4_t));
  for (int i = 0; i < length; ++i)
    stack_ucs4_grow (heap, c);
  stack_align (heap);
  return stack_finish (heap) | POINTER_TYPE;
}

uint32_t *
string_bytes (Object string)
{
  return (uint32_t *) (((Pointer) string) + 1);
}

size_t
string_length (Object string)
{
  return ((Pointer) string)[0] / sizeof (ucs4_t);
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
  stack_grow (heap, STRING_TYPE);
  stack_grow (heap, 0);
  for (n = 0; !is_null (chars); chars = cdr (chars), ++n)
    stack_ucs4_grow (heap, char_value (car (chars)));
  stack_align (heap);
  Object str = stack_finish (heap) | POINTER_TYPE;
  ((Pointer) str)[0] = n * sizeof (ucs4_t);
  return str;  
}

Object
make_symbol (Heap *heap, uint8_t *s, size_t len)
{
  stack_grow (heap, SYMBOL_TYPE);
  stack_grow (heap, len * sizeof (uint8_t));
  stack_utf8_grow (heap, s, len);
  stack_align (heap);
  Object sym = stack_finish (heap) | POINTER_TYPE;
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
  return ((Pointer) sym)[0] / sizeof (uint8_t);
}

Object
symbol (Heap *heap, Object chars)
{
  int n;
  uint8_t s[6];
  stack_grow (heap, SYMBOL_TYPE);
  stack_grow (heap, 0);
  for (n = 0; !is_null (chars); chars = cdr (chars), ++n)
    {
      size_t len = u8_uctomb (s, char_value (car (chars)), 6);
      stack_utf8_grow (heap, s, len);
    }
  stack_align (heap);
  Object sym = stack_finish (heap) | POINTER_TYPE;
  ((Pointer) sym)[0] = n * sizeof (uint8_t);
  return symbol_table_intern (&heap->symbol_table, sym, false);
}

Object
make_vector (Heap *heap, size_t length, Object object)
{
  stack_grow (heap, VECTOR_TYPE);
  stack_grow (heap, WORDSIZE * length);
  for (int i = 0; i < length; ++i)
    stack_grow (heap, object);
  stack_align (heap);
  return stack_finish (heap) | POINTER_TYPE;
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
