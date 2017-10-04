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

#ifndef COMPILER_LIVENESS_H_INCLUDED
#define COMPILER_LIVENESS_H_INCLUDED

#include <stddef.h>

#include "variable.h"

#define use_foreach(use, var)						\
  vector_foreach (use, UseChain, Use, use_chain, variable_use_chain (var))

void program_init_liveness (struct program *program);
void program_init_def_use_chains (struct program *program);

bool variable_live_at (struct program *program, struct block *block,
		       size_t time, struct variable *v);

#endif /* COMPILER_LIVENESS_H_INCLUDED */
