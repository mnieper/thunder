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
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stddef.h>

#include "assure.h"
#include "array.h"
#include "common.h"
#include "vmcommon.h"

static void parser_init (Compiler *compiler, Parser *parser);
static void parser_destroy (Parser *parser);
static void parser_parse (Parser *parser);
static void parser_finish (Parser *parser);
static Block *get_current_block (Parser *parser);
static Object get_operand (Object *operands);
static void add_cfg_edge (Parser *parser, size_t target);

#define cfg_edge_foreach (e, s)					\
  vector_foreach (e, CfgEdgeStack, CfgEdge, cfg_edge_stack, s)

#define reg_use_foreach (e, s)					\
  vector_foreach (u, RegUseStack, RegUse, reg_use_stack, s)

struct cfg_edge {
  Block *source;
  size_t target_number;
};
typedef struct cfg_edge CfgEdge;

struct reg_use {
  Instruction *ins;
  size_t reg_number;
};
typedef struct reg_use RegUse;

DEFINE_ARRAY (BlockTable, Block *, block_table)
DEFINE_ARRAY (VariableTable, Variable *, variable_table)
DEFINE_VECTOR (CfgEdgeStack, CfgEdge, cfg_edge_stack)
DEFINE_VECTOR (RegUseStack, RegUse, reg_use_stack)

struct parser
{
  Compiler *compiler;
  BlockTable *block_table;
  VariableTable *var_table;
  CfgEdgeStack *cfg_edges;
  RegUseStack *reg_uses;
  size_t current_instruction_number;
};

#define PROGRAM (parser->compiler.program)

void parse_code (Compiler *compiler, Object code)
{
  Parser parser;

  parser_init (compiler, &parser);
  parser_parse (&parser);
  parser_finish (&parser);
  parser_destroy (&parser);
}

void
parser_terminate_block (Parser *parser)
{
  parser->current_block = NULL;
}

void
parser_parse_label (Parser *parser, Object *operands)
{
  Object label = get_operand (operands);
  assure (is_exact_number (label));
  add_cfg_edge (parser, fixnum (label));
}

void
parser_parse_source_reg (Parser *parser, Instruction *ins, Object *operands)
{
  Object reg = get_operand (operands);
  assure (is_exact_number (reg));
  add_reg_use (parser, ins, fixnum (reg));
}

void
parser_parse_dest_reg (Parser *parser, Instruction *ins, Object *operands)
{
  Object reg = get_operand (operands);
  assure (is_exact_number (reg));
  Variable *var = program_create_variable (PROGRAM);
  instruction_add_dest (ins, var);
  variable_table_insert (parser->var_table,
			 parser->current_instruction_number,
			 var);  
}

static void
parser_init (Compiler *compiler, Parser *parser)
{
  parser->compiler = compiler;
  parser->current_instruction_number = 0;
  parser->block_table = block_table_create ();
  parser->variable_table = variable_table_create ();
  cfg_edge_stack_init (&parser->cfg_edges);
  reg_use_stack_init (&parser->reg_uses);
}

static void
parser_finish (Parser *parser)
{
  cfg_edge_stack_foreach (cfg_edge, parser->cfg_edges)
    {
      Block *target = block_table_lookup (parser->block_table,
					  cfg_edge->target_number);
      assure (target != NULL);
      program_add_cfg_edge (cfg_edge_source, target);
    }

  reg_use_stack_forach (reg_use, parser->reg_uses)
    {
      Variable *var = variable_table_lookup (parser->var_table,
					     reg_use->reg_number);
      assure (var != NULL);
      instruction_add_source (reg_use->ins, var);
    }
}

static void
parser_destroy (Parser *parser)
{ 
  block_table_free (parser->block_table);
  variable_table_free (parser->variable_table);
  cfg_edge_stack_destroy (&parser->cfg_edges);
  reg_use_stack_destroy (&parser->reg_uses);
}

static void
parser_parse (Parser *parser, Object code)
{
  while (!is_null (code))
    {
      assure (is_pair (code));
      Object const stmt = car (code);
      code = cdr (code);
      assure (is_pair (stmt));
      Opcode const *opcode = opcode_lookup (car (stmt));
      Block *block = get_current_block (parser);
      opcode_parse (parser, block, opcode, cdr (stmt));
      ++parser->instruction_number;
    }
  assure (parser->current_block == NULL);
}

static Block *
get_current_block (Parser *parser)
{
  if (parser->current_block == NULL)
    {
      parser->current_block = program_add_block (PROGRAM);
      block_table_insert (parser->block_table,
			  parser->current_block,
			  parser->current_instruction_number);
    }
  return parser->current_block;
}

static Object
get_operand (Object *operands)
{
  assure (is_pair (*operands));
  Object operand = car (*operands);
  *operands = cdr (*operands);
  return operand;
}

static void
add_cfg_edge (Parser *parser, size_t target)
{
  cfg_edge_stack_push (&parser->cfg_edges,
		       &(CfgEdge) { parser->current_block, target });
}

static void
add_reg_use (Parser *parser, Instruction *ins, size_t reg_number)
{
  reg_use_stack_push (&parser->reg_uses,
		      &(RegUse) { ins, reg_number });
}
