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

#define variable_foreach(v, l)					\
  list_foreach (v, VariableList, Variable, variable_list, l)

typedef struct variable Variable;

struct compiler;

Variable *variable_create (struct compiler *compiler);
void variable_free (Variable *var);

DEFINE_LIST(VariableList, Variable, variable_list, variable_free);

#endif /* COMPILER_VARIABLE_H_INCLUDES */
