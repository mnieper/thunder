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

#ifndef RUNTIME_H_INCLUDED
#define RUNTIME_H_INCLUDED

#include "vmcommon.h"

#define list(heap, ...)				\
  runtime_list (heap, __VA_ARGS__, NONE)

Object
runtime_list (Heap *heap, ...);

#endif /* RUNTIME_H_INCLUDED */
