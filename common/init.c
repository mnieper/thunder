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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <ltdl.h>
#include <stdbool.h>
#include <stdlib.h>

#include "error.h"
#include "vmcommon.h"

static bool initialized = false;

static void
finish (void)
{
  finish_compiler ();
  finish_symbols ();
  lt_dlexit ();
}

void
init (void)
{
  if (initialized)
    return;

  LTDL_SET_PRELOADED_SYMBOLS ();
  if (lt_dlinit () != 0)
    error (EXIT_FAILURE, 0, "%s", lt_dlerror ());

  init_symbols ();
  init_compiler ();

  initialized = true;
  atexit (finish);
}
