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


#ifndef LIBTHUNDER_H_INCLUDED
#define LIBTHUNDER_H_INCLUDED

#include <stdio.h>

typedef struct vm Vm;

void
vm_init (void);

Vm *
vm_create (void);

void
vm_free (Vm *);

int
vm_load (Vm *, FILE *, char const *);

#endif /* LIBTHUNDER_H_INCLUDED */
