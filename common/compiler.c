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
#include <lightning.h>
#include <ltdl.h>
#include <stdbool.h>
#include <stddef.h>

#include "bitrotate.h"
#include "compiler.h"
#include "error.h"
#include "hash.h"
#include "progname.h"
#include "stack.h"
#include "vmcommon.h"
#include "xalloc.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

#define FRAME_SIZE 256  /* TODO: Calculate correct value. */

typedef STACK(jit_node_t *) EntryPointStack;

static lt_dlhandle module;
static jit_pointer_t *trampoline_return;

/* Instruction definitions */

static void
assert_operand (Object operands)
{
  if (is_null (operands))
    error (EXIT_FAILURE, 0, "%s", "missing operand");
}

#define OPERAND_TYPE(type) OPERAND_TYPE_##type
#define OPERAND_TYPE_ireg jit_gpr_t
#define OPERAND_TYPE_imm jit_word_t
#define OPERAND_TYPE_fun jit_pointer_t

#define OPERAND(var, type)					\
  assert_operand (operands);					\
  OPERAND_TYPE(type) var = get_##type (data, car (operands));	\
  operands = cdr (operands)

static jit_gpr_t
get_ireg (struct obstack *data, Object operand)
{
  if (operand == SYMBOL(R0))
    return JIT_R0;
  if (operand == SYMBOL(R1))
    return JIT_R1;
  if (operand == SYMBOL(R2))
    return JIT_R2;
  if (operand == SYMBOL(V0))
    return JIT_V0;
  if (operand == SYMBOL(V1))
    return JIT_V1;
  if (operand == SYMBOL(V2))
    return JIT_V2;
  error (EXIT_FAILURE, 0, "%s: %s", "not an integer register", object_get_str (operand));
}

static jit_word_t
get_imm (struct obstack *data, Object operand)
{
  if (is_symbol (operand))
    {
      char *s = symbol_value (operand);
      jit_word_t value;
      switch (s[0])
	{
	case '&':
	  {
	    jit_pointer_t address = lt_dlsym (module, s + 1);
	    if (address == NULL)
	      error (EXIT_FAILURE, 0, "%s", lt_dlerror ());
	    value = (jit_word_t) address;
	  }
	  break;
	default:
	  error (EXIT_FAILURE, 0, "%s: %s", "not an immediate value", object_get_str (operand));
	}
      free (s);
      return value;
    }

  if (is_string (operand))
    {
      char *s = string_value (operand);
      jit_word_t value = (jit_word_t) obstack_copy0 (data, s, strlen (s));
      free (s);
      return value;
    }

  if (is_char (operand))
    {
      return char_value (operand);
    }
  
  /* TODO: Check for an integer. */
  if (!is_exact_number (operand))
    error (EXIT_FAILURE, 0, "%s: %s", "not an exact number", object_get_str (operand));
  return fixnum (operand);
}

static jit_pointer_t
get_fun (struct obstack *data, Object operand)
{
  return (jit_pointer_t) get_imm (data, operand);
}

#define DEFINE_INSTRUCTION(name)			\
  static void						\
  instruction_##name (jit_state_t *_jit,		\
		      EntryPointStack *entry_points,	\
		      struct obstack *data,		\
		      Object operands)

#define DEFINE_INSTRUCTION_void(name)		\
  DEFINE_INSTRUCTION(name)			\
  {						\
    jit_##name ();				\
  }

#define DEFINE_INSTRUCTION_fn(name)		\
  DEFINE_INSTRUCTION(name)			\
  {						\
    OPERAND (fn, fun);				\
    jit_##name (fn);				\
  }

#define DEFINE_INSTRUCTION_im(name)		\
  DEFINE_INSTRUCTION(name)			\
  {						\
    OPERAND (im, imm);				\
    jit_##name (im);				\
  }

#define DEFINE_INSTRUCTION_ir(name)		\
  DEFINE_INSTRUCTION(name)			\
  {						\
    OPERAND (ir, ireg);				\
    jit_##name (ir);				\
  }

#define DEFINE_INSTRUCTION_ir_im(name)		\
  DEFINE_INSTRUCTION(name)			\
  {						\
    OPERAND (r0, ireg);				\
    OPERAND (im, imm);				\
    jit_##name (r0, im);				\
  }

#define DEFINE_INSTRUCTION_ir_ir(name)		\
  DEFINE_INSTRUCTION(name)			\
  {						\
    OPERAND (r0, ireg);				\
    OPERAND (r1, ireg);				\
    jit_##name (r0, r1);				\
  }

#define DEFINE_INSTRUCTION_ir_ir_im(name)	\
  DEFINE_INSTRUCTION(name)			\
  {						\
    OPERAND (r0, ireg);				\
    OPERAND (r1, ireg);				\
    OPERAND (im, imm);				\
    jit_##name (r0, r1, im);			\
  }

#define DEFINE_INSTRUCTION_ir_ir_ir(name)	\
  DEFINE_INSTRUCTION(name)			\
  {						\
    OPERAND (r0, ireg);				\
    OPERAND (r1, ireg);				\
    OPERAND (r2, ireg);				\
    jit_##name (r0, r1, r2);			\
  }

DEFINE_INSTRUCTION (entry)
{
  jit_node_t *label;
  label = jit_indirect ();
  stack_push (entry_points, label);
}

DEFINE_INSTRUCTION(ret)
{
  jit_node_t *jump;
  jump = jit_jmpi ();
  jit_patch_abs (jump, trampoline_return);
}

#define DEFINE_INSTRUCTION_special(name)

#define EXPAND_INSTRUCTION(name, type)		\
  DEFINE_INSTRUCTION_##type (name)
#include "instructions.def"
#undef EXPAND_INSTRUCTION

enum {
#define EXPAND_INSTRUCTION(name, type)		\
  INSTRUCTION_INDEX_##name,
# include "instructions.def"
#undef EXPAND_INSTRUCTION
  INSTRUCTION_COUNT
};

/* Instruction table */
typedef Hash_table InstructionTable;

static InstructionTable *instruction_table;

typedef void (*InstructionFunction) (jit_state_t *_jit,		
				     EntryPointStack *entry_points,
				     struct obstack *data,
				     Object operands);

struct instruction
{
  Object name;
  InstructionFunction function;
};

static bool
instruction_comparator (void const *entry1, void const *entry2)
{
  return ((struct instruction const *) entry1)->name
    == ((struct instruction const *) entry2)->name;
}

static size_t
instruction_hasher (void const *entry, size_t n)
{
  size_t val = rotr_sz ((size_t) ((struct instruction const *) entry)->name, 3);
  return val % n;
}

static void
instruction_insert (InstructionTable *table, Object name, InstructionFunction function)
{
  struct instruction *instr = XMALLOC (struct instruction);
  instr->name = name;
  instr->function = function;
  if (hash_insert (table, instr) == NULL)
    xalloc_die ();
}

static InstructionTable *
instruction_table_create ()
{
  InstructionTable *table
    = hash_initialize (INSTRUCTION_COUNT, NULL, instruction_hasher, instruction_comparator, free);
#define EXPAND_INSTRUCTION(name, type)					\
  instruction_insert (table, INSTRUCTION(name), instruction_##name);
# include "instructions.def"
#undef EXPAND_INSTRUCTION
  return table;
}

static void
instruction_table_free (InstructionTable *table)
{
  hash_free (table);
}

static InstructionFunction
instruction_table_lookup (InstructionTable *table, Object name)
{
  struct instruction *instr
    = hash_lookup (table, &(struct instruction) { .name = name });
  if (!instr)
    error (EXIT_FAILURE, 0, "%s: %s", "unknown instruction", object_get_str (name));
  return instr->function;
}


/* Label table used by compiler */
struct label
{
  Object name;
  jit_node_t *jit_label;
};

typedef Hash_table LabelTable;

static bool
comparator (void const *entry1, void const *entry2)
{
  return ((struct label const *) entry1)->name
    == ((struct label const *) entry2)->name;
}

static size_t
hasher (void const *entry, size_t n)
{
  size_t val = rotr_sz ((size_t) ((struct label const *) entry)->name, 3);
  return val % n;
}

static LabelTable *
label_table_create ()
{
  return hash_initialize (0, NULL, hasher, comparator, free);
}

static void
label_table_free (LabelTable *table)
{
  hash_free (table);
}

int (*trampoline) (void *f, void *arg);

static jit_state_t *_jit;

void
init_compiler (void)
{
  jit_node_t *f;
  jit_node_t *arg;
  jit_node_t *done;
  
  jit_set_memory_functions (xmalloc, xrealloc, free);
  init_jit (program_name);

  _jit = jit_new_state ();
  jit_prolog ();
  jit_frame (FRAME_SIZE);
  f = jit_arg ();
  arg = jit_arg ();
  jit_getarg (JIT_R1, f);
  jit_getarg (JIT_R0, arg);
  jit_jmpr (JIT_R1);
  done = jit_indirect ();
  jit_retr (JIT_R0);
  jit_epilog ();
  trampoline = jit_emit();
  trampoline_return = jit_address (done);  
  jit_clear_state ();

  instruction_table = instruction_table_create ();

  module = lt_dlopen (NULL);
}

void
finish_compiler (void)
{
  instruction_table_free (instruction_table);
  
  jit_destroy_state ();
  finish_jit ();
}

void
assembly_init (Assembly assembly)
{
  assembly->clear = true;
}

void
assembly_clear (Assembly assembly)
{
#define _jit (assembly->jit)
  assembly_destroy (assembly);
  assembly->jit = jit_new_state ();
  obstack_init (&assembly->data);
  assembly->clear = false;
#undef _jit
}

void
assembly_destroy (Assembly assembly)
{
#define _jit (assembly->jit)
  if (assembly->clear)
    return;

  jit_destroy_state ();
  obstack_free (&assembly->data, NULL);
  free (assembly->entry_points);
  
  assembly->clear = true;
#undef _jit
}

Object
compile (Heap *heap, Object code)
{
#define assembly (RESOURCE_PAYLOAD (res))
#define _jit assembly->jit
  Resource(ASSEMBLY) *res = resource_manager_allocate (ASSEMBLY, &heap->resource_manager);

  assembly_clear (assembly);

  LabelTable *labels = label_table_create ();

  EntryPointStack entry_points;
  stack_init (&entry_points);
  
  jit_prolog ();
  jit_tramp (FRAME_SIZE);

  assert_list (code);
  while (!is_null (code))
    {
      Object stmt = car (code);
      if (is_pair (stmt))
	{
	  assert_list (stmt);
	  
	  Object op = car (stmt);
	  assert_symbol (op);
	  InstructionFunction fun = instruction_table_lookup (instruction_table, op);
	  fun (_jit, &entry_points, &assembly->data, cdr (stmt));
	}
      else
	{
	  assert_symbol (stmt);
	}

      code = cdr (code);
    }
  jit_epilog ();

  jit_emit ();
  assembly->entry_point_number = stack_size (&entry_points);
  assembly->entry_points = XNMALLOC (assembly->entry_point_number, EntryPoint);
  {
    jit_node_t **label;
    EntryPoint *p = assembly->entry_points;
    STACK_FOREACH (&entry_points, label)
      {
	*p = jit_address (*label);
	++p;
      }
  }
  jit_disassemble (); // XXX
  jit_clear_state ();

  label_table_free (labels);
  stack_destroy (&entry_points);
  
  return (Object) res | POINTER_TYPE;
#undef _jit
#undef assembly
}

bool
is_assembly (Object obj)
{
  return (obj & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) obj)[-1] & HEADER_TYPE_MASK) == ASSEMBLY_TYPE;
}

size_t
assembly_entry_point_number (Object assembly)
{
  return (*((Assembly *) assembly)) [0].entry_point_number;
}

EntryPoint *
assembly_entry_points (Object assembly)
{
  return (*((Assembly *) assembly)) [0].entry_points;
}

int
call (Object assembly, size_t entry, void *data)
{  
  return trampoline ((*((Assembly *) assembly)) [0].entry_points [entry], data);
}
