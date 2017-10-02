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

#ifndef COMPILER_ERROR_H_INCLUDED
#define COMPILER_ERROR_H_INCLUDED

#include <stdlib.h>

#include "../error.h"

#define compiler_error(format, ...)		\
  (error (EXIT_FAILURE, 0, format, __VA_ARGS__), abort ())

#endif /* COMPILER_ERROR_H_INCLUDED */
