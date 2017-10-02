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
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "bitrotate.h"
#include "compiler/common.h"
#include "compiler/error.h"
#include "compiler/instruction.h"
#include "compiler/value.h"
#include "hash.h"
#include "vmcommon.h"
#include "xalloc.h"

/* Opcodes */
static void
check_operand (Object operands)
{
  if (is_null (operands))
    compiler_error ("%s", "missing operand");
}

#define GET_VALUE(value)						\
  do									\
    {									\
      check_operand (operands);						\
      (value) = new_value (compiler, car (operands), NULL);		\
      operands = cdr (operands);					\
    }									\
  while (0)

#define GET_LABEL(value)						\
  do									\
    {									\
      check_operand (operands);						\
      (value) = get_label (compiler, car (operands));			\
      operands = cdr (operands);					\
    }									\
  while (0)

#define GET_LVALUE(value)						\
  do									\
    {									\
      check_operand (operands);						\
      (value) = new_value (compiler, car (operands), instruction);	\
      operands = cdr (operands);					\
    }									\
  while (0)

static void
instruction_w_out_str (FILE *out, Instruction *ins)
{
  char *s = object_get_str (ins->opcode);
  fprintf (out, "%s", s);
  free (s);

  fputc (' ', out);
  value_out_str (out, ins->u);
}

static void
instruction_www_patch (Instruction *ins)
{
  /* TODO */
}

static void
instruction_www_out_str (FILE *out, Instruction *ins)
{
  char *s = object_get_str (ins->opcode);
  fprintf (out, "%s", s);
  free (s);

  fputc (' ', out);
  value_out_str (out, ins->u);
  fputc (' ', out);
  value_out_str (out, ins->v);
  fputc (' ', out);
  value_out_str (out, ins->w);
}

static Instruction *
new_instruction_www (Compiler *compiler, Object operands,
		     Jitter jit)
{
  Instruction *instruction = ALLOC (struct instruction);
  instruction->type = INSTRUCTION_NORMAL;
  instruction->jit = jit;
  instruction->patch = instruction_www_patch;
  instruction->out_str = instruction_www_out_str;
  instruction->patched = false;
  GET_LVALUE (instruction->u);
  GET_VALUE (instruction->v);
  GET_VALUE (instruction->w);
  return instruction;
}

#define DEFINE_OPCODE_www(name)						\
  static void								\
  instruction_##name##_jit (jit_state_t *_jit, Instruction *ins)		\
  {									\
    if (value_immediate (ins->w))					\
      jit_##name##i (ins->u.gpr, ins->v.gpr, ins->w.word);		\
    else								\
      jit_##name##r (ins->u.gpr, ins->v.gpr, ins->w.gpr);		\
  }									\
									\
  static Instruction *							\
  new_instruction_##name (Compiler *compiler, Object operands)		\
  {									\
    return new_instruction_www (compiler, operands,			\
				instruction_##name##_jit);		\
  }

static void
instruction_pww_patch (Instruction *ins)
{
  ins->patched = true;
  ins->u.type = VALUE_BLOCK;
  ins->u.block = ins->u.label->block;
}

static void
instruction_pww_out_str (FILE *out, Instruction *ins)
{
  char *s = object_get_str (ins->opcode);
  fprintf (out, "%s", s);
  free (s);

  fputc (' ', out);
  value_out_str (out, ins->u);
  fputc (' ', out);
  value_out_str (out, ins->w);
  fputc (' ', out);
  value_out_str (out, ins->x);
}

static Instruction *
new_instruction_pww (Compiler *compiler, Object operands, Jitter jit)
{
  Instruction *instruction = ALLOC (struct instruction);
  instruction->type = INSTRUCTION_BRANCH;
  instruction->jit = jit;
  instruction->patch = instruction_pww_patch;
  instruction->out_str = instruction_pww_out_str;
  instruction->patched = false;
  GET_LABEL (instruction->u);
  GET_VALUE (instruction->w);
  GET_VALUE (instruction->x);
  return instruction;
}

#define DEFINE_OPCODE_pww(name)			\
  static void								\
  instruction_##name##_jit (jit_state_t *_jit, Instruction *ins)		\
  {									\
    /* TODO */								\
  }									\
									\
  static Instruction *							\
  new_instruction_##name (Compiler *compiler, Object operands)		\
  {									\
    return new_instruction_pww (compiler, operands,			\
				instruction_##name##_jit);		\
  }

static void
instruction_p_patch (Instruction *ins)
{
  ins->patched = true;
  ins->u.type = VALUE_BLOCK;
  ins->u.block = ins->u.label->block;
}

static void
instruction_p_out_str (FILE *out, Instruction *ins)
{
  char *s = object_get_str (ins->opcode);
  fprintf (out, "%s", s);
  free (s);

  fputc (' ', out);
  value_out_str (out, ins->u);
}

static Instruction *
new_instruction_p (Compiler *compiler, Object operands, Jitter jit)
{
  Instruction *instruction = ALLOC (struct instruction);
  instruction->type = INSTRUCTION_JUMP;
  instruction->jit = jit;
  instruction->patch = instruction_p_patch;
  instruction->out_str = instruction_p_out_str;
  instruction->patched = false;
  GET_LABEL (instruction->u);
  return instruction;
}

#define DEFINE_OPCODE_p(name)			\
  static void								\
  instruction_##name##_jit (jit_state_t *_jit, Instruction *ins)		\
  {									\
    /* TODO */								\
  }									\
									\
  static Instruction *							\
  new_instruction_##name (Compiler *compiler, Object operands)		\
  {									\
    return new_instruction_p (compiler, operands,			\
			      instruction_##name##_jit);		\
  }

#define DEFINE_OPCODE_ret(name)						\
  static void								\
  instruction_##name##_jit (jit_state_t *_jit, Instruction *ins)		\
  {									\
    /* TODO */								\
  }									\
  									\
  static void								\
  instruction_##name##_patch (Instruction *ins)				\
  {									\
  }									\
									\
  static Instruction *							\
  new_instruction_##name (Compiler *compiler, Object operands)		\
  {									\
    Instruction *instruction = ALLOC (struct instruction);		\
    instruction->type = INSTRUCTION_RETURN;				\
    instruction->jit = instruction_##name##_jit;			\
    instruction->out_str = instruction_w_out_str;			\
    instruction->patch = instruction_##name##_patch;			\
    instruction->patched = false;					\
    GET_VALUE (instruction->u);						\
    return instruction;							\
  }

#define EXPAND_OPCODE(name, type)		\
  DEFINE_OPCODE_##type (name)
#include "opcodes.def"
#undef EXPAND_OPCODE

/* Opcode table */
enum {
# define EXPAND_OPCODE(name, type)		\
  OPCODE_INDEX_##name,
#  include "opcodes.def"
# undef EXPAND_OPCODE
  OPCODE_COUNT
};

typedef Hash_table *restrict OpcodeTable;

static OpcodeTable opcode_table;

typedef Instruction *(*InstructionConstructor) (Compiler *compiler,
						Object operands);

struct opcode
{
  Object name;
  InstructionConstructor constructor;
};

#define OPCODE_PTR(p) ((struct opcode const *) (p))

static bool
opcode_comparator (void const *entry1, void const *entry2)
{
  return OPCODE_PTR(entry1)->name == OPCODE_PTR(entry2)->name;
}

static size_t
opcode_hasher (void const *entry, size_t n)
{
  size_t val = rotr_sz ((size_t) OPCODE_PTR(entry)->name, 3);
  return val % n;
}

#undef OPCODE_PTR

static void
opcode_insert (Object name, InstructionConstructor constructor)
{
  struct opcode *opcode = XMALLOC (struct opcode);
  opcode->name = name;
  opcode->constructor = constructor;
  if (hash_insert (opcode_table, opcode) == NULL)
    xalloc_die ();
}

static InstructionConstructor
opcode_table_lookup (Object name)
{
  struct opcode *opcode
    = hash_lookup (opcode_table, &(struct opcode) { .name = name });
  if (opcode == NULL)
    compiler_error ("%s: %s", "unknown instruction", object_get_str (name));
  return opcode->constructor;
}

static void
opcode_table_create (void)
{
  opcode_table = hash_initialize (OPCODE_COUNT, NULL, opcode_hasher,
				  opcode_comparator, free);
# define EXPAND_OPCODE(name, type)			\
  opcode_insert (OPCODE(name), new_instruction_##name);
#  include "opcodes.def"
# undef EXPAND_OPCODE
}

static void
opcode_table_free (void)
{
  hash_free (opcode_table);
}

/* Instructions */

void
init_instructions (void)
{
  opcode_table_create ();
}

void
finish_instructions (void)
{
  opcode_table_free ();
}

Instruction *
new_instruction (Compiler *compiler, Object stmt)
{
  InstructionConstructor constructor = opcode_table_lookup (car (stmt));
  Instruction *ins = constructor (compiler, cdr (stmt));
  ins->opcode = car (stmt);
  return ins;
}

Instruction *
new_jump (Compiler *compiler, Block *target)
{
  Instruction *ins = ALLOC (Instruction);
  ins->opcode = OPCODE(jmp);
  ins->type = INSTRUCTION_JUMP;
  ins->out_str = instruction_p_out_str;
  ins->u = block_value (target);
  ins->patched = true;
  return ins;
}

Instruction *
instruction_next (Instruction *ins)
{
  return deque_next (INSTRUCTIONS, ins);
}

bool
instruction_jmp (Instruction *ins)
{
  return ins->type == INSTRUCTION_JUMP;
}

bool
instruction_branch (Instruction *ins)
{
  return ins->type == INSTRUCTION_BRANCH;
}

bool
instruction_terminating (Instruction *ins)
{
  switch (ins->type)
    {
    case INSTRUCTION_JUMP:
    case INSTRUCTION_BRANCH:
    case INSTRUCTION_RETURN:
    case INSTRUCTION_GOTO:
      return true;
    }
  return false;
}

void
instruction_patch (Compiler *compiler, Instruction *ins)
{
  if (ins->patched)
    return;
  ins->patched = true;
  ins->patch (ins);
}

void
instruction_out_str (FILE *out, Instruction *ins)
{
  ins->out_str (out, ins);
}
