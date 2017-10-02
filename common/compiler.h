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

#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include "heap.h"
#include "libthunder.h"
#include "object.h"

void init_compiler (void);
void finish_compiler (void);

Object compile (Heap *heap, Object code);

extern int (*trampoline) (Vm *vm, void *f, void *arg);

#endif /* COMPILER_H_INCLUDED */
