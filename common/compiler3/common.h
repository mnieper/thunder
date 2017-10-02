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

#ifndef COMPILER_COMMON_H_INCLUDED
#define COMPILER_COMMON_H_INCLUDED

#include <lightning.h>
#include <ltdl.h>
#include <stdbool.h>
#include <stdio.h>

#include "deque.h"
#include "graph.h"
#include "hash.h"
#include "obstack.h"
#include "vmcommon.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

/* Forward declarations */
typedef struct block Block;
typedef struct instruction Instruction;

/* Globals */
extern lt_dlhandle module;

/* Obstack */
#define ALLOC(t)						\
  ((t *) (obstack_alloc (&compiler->stack, sizeof (t))));

/* Labels */
struct label
{
  Object name;
  Block *block;
};

typedef struct label Label;

typedef Hash_table LabelTable;

/* Registers */
struct reg
{
  Object name;
  Instruction *def;
};

typedef struct reg Register;

typedef Hash_table RegisterTable;

/* Values */
struct value
{
  enum
    {
      VALUE_LABEL,
      VALUE_BLOCK,
      VALUE_REGISTER,
      VALUE_GPR,
      VALUE_FPR,
      VALUE_IMMEDIATE,
      VALUE_FLOAT
    } type;
  union
  {
    Block *block;
    Label *label;
    Register *reg;
    jit_gpr_t gpr;
    jit_fpr_t fpr;
    jit_word_t word;
    jit_float64_t float64;
  };
};
typedef struct value Value;

/* Instructions */
typedef void (*Jitter) (jit_state_t *_jit, Instruction *ins);

#define INSTRUCTIONS DEQUE(Instruction, instructions)
struct instruction
{
  Object opcode;
  enum
    {
      INSTRUCTION_NORMAL,
      INSTRUCTION_JUMP,
      INSTRUCTION_BRANCH,
      INSTRUCTION_RETURN,
      INSTRUCTION_GOTO
    } type;
  Jitter jit;
  void (*patch) (Instruction *ins);
  void (*out_str) (FILE *out, Instruction *ins);
  struct deque_entry instructions;
  union
  {
    struct
    {
      Value u;
      Value v;
      Value w;
      Value x;
    };
    Block *target;
  };
  bool patched : 1;
};
typedef Deque InstructionList;

/* Blocks */

struct link
{
  Value *source;
  Edge links;
};
typedef struct link Link;

struct block
{
  Object name;
  InstructionList instructions;
  Node blocks;
  size_t length;
};

/* Compiler */
#define COMPILER_ASSEMBLY(compiler) (*(compiler)->assembly)
#define COMPILER_JIT(compiler)  ((*((compiler)->assembly))->jit)
#define COMPILER_DATA(compiler) ((*((compiler)->assembly))->data)

typedef Graph Cfg;

typedef struct compiler Compiler;
struct compiler
{
  Assembly *restrict assembly;
  struct obstack stack;
  LabelTable *labels;
  RegisterTable *registers;
  struct obstack data;
  Cfg cfg;
};

#endif /* COMPILER_COMMON_H_INCLUDED */
