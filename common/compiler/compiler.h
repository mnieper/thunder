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

#ifndef COMPILER_COMPILER_H_INCLUDED
#define COMPILER_COMPILER_H_INCLUDED

#include <stddef.h>

#include "obstack.h"
#include "program.h"

#define COMPILER_ALLOC(compiler, t)				\
  ((t *) (compiler_alloc ((compiler), sizeof (t))))
#define COMPILER_NALLOC(compiler, n, t)		\
  ((t *) (compiler_alloc ((compiler), n * sizeof (t))))

typedef struct compiler Compiler;

void compiler_init (Compiler *compiler);
void compiler_destroy (Compiler *compiler);

void *compiler_alloc (Compiler *compiler, size_t size);

struct compiler
{
  struct obstack obstack;
  Program program;
};

#endif /* COMPILER_COMPILER_H_INCLUDED */
