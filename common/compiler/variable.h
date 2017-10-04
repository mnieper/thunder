/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifndef COMPILER_VARIABLE_H_INCLUDED
#define COMPILER_VARIABLE_H_INCLUDED

#include "list.h"
#include "vector.h"

#define variable_foreach(v, l)					\
  list_foreach (v, VariableList, Variable, variable_list, l)

typedef struct variable Variable;

struct compiler;

Variable *variable_create (struct compiler *compiler);
void variable_free (Variable *var);

static inline struct use_chain *variable_use_chain (Variable *var);
static inline struct variable_vector *
variable_congruence_class (Variable *var);

#define VARIABLE_INDEX(var)      ((var)->varindex)
#define VARIABLE_DEF_BLOCK(var)  ((var)->def_block)
#define VARIABLE_DEF_TIME(var)   ((var)->def_time)
#define VARIABLE_CONGRUENCE(var) ((var)->congruence)

DEFINE_LIST(VariableList, Variable, variable_list, variable_free);

typedef struct use Use;
struct use {
  struct block *block;
  size_t last_use_time;
};
DEFINE_VECTOR (UseChain, Use, use_chain)

DEFINE_VECTOR (VariableVector, Variable *, variable_vector)

struct variable
{
  size_t varindex;
  struct block *def_block;
  size_t def_time;
  UseChain use_chain;
  Variable *congruence;
  VariableVector congruence_class;
};

static inline UseChain *
variable_use_chain (Variable *var)
{
  return &var->use_chain;
}

inline struct variable_vector *
variable_congruence_class (Variable *var)
{
  return &var->congruence_class;
}

#endif /* COMPILER_VARIABLE_H_INCLUDES */
