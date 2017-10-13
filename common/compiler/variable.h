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

#include <stdbool.h>

#include "list.h"
#include "vector.h"

#define variable_foreach(v, l)					\
  list_foreach (v, VariableList, Variable, variable_list, l)
#define variable_pos_foreach(v, p, l)					\
  list_pos_foreach (v, p, VariableList, Variable, variable_list, l)

typedef struct variable Variable;

struct compiler;

Variable *variable_create (struct compiler *compiler);
void variable_free (Variable *var);

static inline struct use_chain *variable_use_chain (Variable *var);
static inline struct variable_vector *
variable_congruence_class (Variable *var);

bool variable_def_dominates (Variable *v, Variable *w);

#define VARIABLE_INDEX(var)              ((var)->varindex)
#define VARIABLE_DEF_BLOCK(var)          ((var)->def_block)
#define VARIABLE_DEF_TIME(var)           ((var)->def_time)
#define VARIABLE_CONGRUENCE(var)         ((var)->congruence)
#define VARIABLE_PARALLEL_COPY_LOC(var)  ((var)->parallel_copy_loc)
#define VARIABLE_PARALLEL_COPY_PRED(var) ((var)->parallel_copy_pred)
#define VARIABLE_PARALLEL_COPY_SEEN(var) ((var)->parallel_copy_seen)
#define VARIABLE_LIVE_FROM(var)          ((var)->live_from)
#define VARIABLE_LIVE_TO(var)            ((var)->live_to)

#ifdef DEBUG
# define VARIABLE_NAME(var) ((var)->name)
#endif

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
  ptrdiff_t varindex;
  struct block *def_block;
  size_t def_time;
  UseChain use_chain;
  Variable *congruence;
  VariableVector congruence_class;
  Variable *parallel_copy_loc;
  Variable *parallel_copy_pred;
  size_t live_from;
  size_t live_to;
  bool parallel_copy_seen : 1;
#ifdef DEBUG
  size_t name;
#endif
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
