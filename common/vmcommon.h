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

#ifndef VM_COMMON_H_INCLUDED
#define VM_COMMON_H_INCLUDED

#include <gmp.h>
#include <lightning.h>
#include <mpfr.h>
#include <mpc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "deque.h"
#include "hash.h"
#include "obstack.h"
#include "unitypes.h"
#include "xalloc.h"

#define SYMBOLS						\
  ENTRY (QUOTE, u8"quote")				\
  ENTRY (QUASIQUOTE, u8"quasiquote")			\
  ENTRY (UNQUOTE, u8"unquote")				\
  ENTRY (UNQUOTE_SPLICING, u8"unquote-splicing")

#define SYMBOL(id)				\
  symbols[id]

#define RESOURCES					\
  ENTRY (EXACT_NUMBER, mpq_t, mpq_init, mpq_clear)	\
  ENTRY (INEXACT_NUMBER, mpc_t, complex_init, mpc_clear)

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

#define WORDSIZE       (sizeof (Object))
#define ALIGNMENT      (2 * WORDSIZE)
#define ALIGNMENT_MASK (ALIGNMENT - 1)

#define OBJECT_TYPE_MASK ALIGNMENT_MASK
#define PAIR_TYPE        0
#define IMMEDIATE_TYPE   1
#define HEADER_TYPE      2
#define LINK_TYPE        3
#define POINTER_TYPE     WORDSIZE
#define MARKED_TYPE      5

#define HEADER_TYPE_MASK  0xffff
#define BINARY_TYPE       0x10
#define UNMANAGED_TYPE    0x20
#define HEADER_SIZE_SHIFT 16
#define MAKE_HEADER_TYPE(type) (type * 0x100 | HEADER_TYPE)
#define HEADER_SIZE(words) (words << HEADER_PAYLOAD_SHIFT)

#define STRING_TYPE         (MAKE_HEADER_TYPE (0) | BINARY_TYPE)
#define SYMBOL_TYPE         (MAKE_HEADER_TYPE (1) | BINARY_TYPE)
#define BYTEVECTOR_TYPE     (MAKE_HEADER_TYPE (2) | BINARY_TYPE)
#define PORT_TYPE           (MAKE_HEADER_TYPE (3) | BINARY_TYPE | HEADER_SIZE (2))
#define EXACT_NUMBER_TYPE   (MAKE_HEADER_TYPE (4) | UNMANAGED_TYPE)
#define INEXACT_NUMBER_TYPE (MAKE_HEADER_TYPE (5) | UNMANAGED_TYPE)
#define EPHEMERON_TYPE      (MAKE_HEADER_TYPE (6) | BINARY_TYPE | HEADER_SIZE (6))
#define CLOSURE_TYPE        MAKE_HEADER_TYPE (7)
#define VECTOR_TYPE         MAKE_HEADER_TYPE (8)
#define RECORD_TYPE         MAKE_HEADER_TYPE (9)

#define IMMEDIATE_TYPE_MASK       0xff
#define IMMEDIATE_PAYLOAD_SHIFT   8
#define MAKE_IMMEDIATE_TYPE(type) (type * (2 * WORDSIZE) | IMMEDIATE_TYPE)

#define BOOLEAN_TYPE MAKE_IMMEDIATE_TYPE (0)
#define CHAR_TYPE    MAKE_IMMEDIATE_TYPE (1)
#define NULL_TYPE    MAKE_IMMEDIATE_TYPE (2)
#define EOF_TYPE     MAKE_IMMEDIATE_TYPE (3)

void *
xaligned_alloc (size_t alignment, size_t size);

typedef jit_uword_t Object;
typedef Object*     Pointer;

bool
is_immediate (Object object);

bool
is_pointer (Object object);

bool
is_unmanaged (Pointer pointer);

Object
header_type (Pointer pointer);

bool
is_binary (Object header);

size_t
object_size (Pointer pointer);

Object *
object_header (Pointer pointer);

Pointer
object_pointers (Pointer header);

Pointer
forwarding_address (Object from_header);

Object
make_mark (Pointer to);

typedef Hash_table MutationTable;

typedef struct symbol_table SymbolTable;
struct symbol_table
{
  Hash_table *nursery_table;
  Hash_table *heap_table;
};

void
symbol_table_init (SymbolTable *restrict symbol_table);

void
symbol_table_destroy (SymbolTable *restrict symbol_table);

Object
symbol_table_intern (SymbolTable *restrict symbol_table, Object sym, bool gc);

Object
symbol_table_clear (SymbolTable *restrict symbol_table, bool major_gc);

/* Resource manager */

#define RESOURCE_PAYLOAD(res)			\
  (res->payload)

#define TYPE(id)         id##_TYPE
#define resource(id)     resource_##id
#define Resource(id)	 Resource_##id
#define ResourceList(id) ResourceList_##id

#define ENTRY(id, type, init, destroy)			\
  typedef struct resource(id) Resource(id);		\
  typedef DEQUE(resource(id)) ResourceList(id);		\
  struct resource(id)					\
  {							\
    Object header;					\
    type payload;					\
    bool in_nursery;					\
    DEQUE_ENTRY(resource(id));				\
  };
RESOURCES
#undef ENTRY

#define nursery_list(id) nursery_list_##id
#define heap_list(id)    heap_list_##id
#define free_list(id)    free_list_##id

typedef struct resource_manager ResourceManager;
struct resource_manager
{
#define ENTRY(id, type, init, destroy)		\
  ResourceList(id) nursery_list(id);		\
  ResourceList(id) heap_list(id);		\
  ResourceList(id) free_list(id);
  RESOURCES
#undef ENTRY
  bool major_gc;
};

void
resource_manager_init (ResourceManager *rm);

void
resource_manager_destroy (ResourceManager *rm);

#define resource_manager_allocate(id, rm) resource_manager_allocate_##id (rm)

#define ENTRY(id, type, init, destroy)			\
  Resource(id) *					\
  resource_manager_allocate_##id (ResourceManager *rm);
RESOURCES
#undef ENTRY

void
resource_manager_begin_gc (ResourceManager *rm, bool major_gc);

void
resource_manager_end_gc (ResourceManager *rm);

#define resource_manager_mark(id, rm, res) resource_manager_mark_##id (rm, res)

#define ENTRY(id, type, init, destroy)		\
  void								\
  resource_manager_mark_##id (ResourceManager *rm, Resource(id) *res);
RESOURCES
#undef ENTRY

/* Heap */

typedef struct heap Heap;
struct heap
{
  Pointer start;
  Pointer free;
  size_t heap_size;
  MutationTable *mutation_table;
  SymbolTable symbol_table;
  ResourceManager resource_manager;
  struct obstack stack;
  void *stack_base;
};

void
heap_init (Heap *heap, size_t heap_size);

void
heap_destroy (Heap *heap);

void
collect (Heap *heap, size_t nursery_size, Object *roots[], size_t root_num);

void
mutate (Heap *heap, Pointer field, Object value);

/* Scheme objects */

Object
make_null ();

bool
is_null (Object object);

Object
make_eof_object ();

bool
is_eof_object (Object object);

Object
make_char (uint32_t c);

uint32_t
char_value (Object c);

bool
is_char (Object object);

Object
make_boolean (bool v);

bool
boolean_value (Object b);

bool
is_boolean (Object obj);

Object
cons (Heap *heap, Object car, Object cdr);

Object
car (Object pair);

Object
cdr (Object pair);

bool
is_pair (Object object);

Object
make_string (Heap *heap, size_t length, ucs4_t c);

uint32_t *
string_bytes (Object s);

size_t
string_length (Object s);

ucs4_t
string_ref (Object string, size_t index);

void
string_set (Object string, size_t index, ucs4_t c);

bool
is_string (Object object);

Object
make_symbol (Heap *heap, uint8_t *s, size_t len);

bool
is_symbol (Object obj);

uint8_t *
symbol_bytes (Object symbol);

size_t
symbol_length (Object symbol);

Object
make_vector (Heap *heap, size_t length, Object object);

size_t
vector_length (Object vector);

Object
vector_ref (Object vector, size_t index);

void
vector_set (Heap *heap, Object vector, size_t index, Object value);

bool
is_vector (Object object);

Object
make_exact_number (Heap *restrict heap, mpq_t q);

bool
is_exact_number (Object object);

void
exact_number_value (mpq_t q, Object num);

Object
make_inexact_number (Heap *restrict heap, mpc_t x);

bool
is_inexact_number (Object object);

void
inexact_number_value (mpc_t x, Object num);

/* Numbers */

void
complex_init (mpc_t x);

void
inexact_to_exact (mpq_t exact, mpfr_t inexact);

/* Scheme reader */

typedef struct reader
{
  void *scanner;
} Reader;

typedef struct location
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} Location;

Object
read_number (Heap *heap, uint8_t const *bytes, size_t length, int radix);

void
reader_init (Reader *restrict reader, Heap *heap, FILE *in);

void
reader_destroy (Reader const *restrict reader);

bool
reader_read (Reader const *restrict reader, Object *restrict object, Location *lloc, char **message);

typedef enum symbol Symbol;
enum symbol
  {
#define ENTRY(id, name)			\
    id,
    SYMBOLS
#undef ENTRY
    SYMBOL_COUNT
  };

extern Object symbols[SYMBOL_COUNT];

#endif /* VM_COMMON_H_INCLUDED */
