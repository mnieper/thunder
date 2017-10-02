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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <lightning.h>
#include <ltdl.h>

#include "compiler/common.h"
#include "progname.h"
#include "vmcommon.h"

#define FRAME_SIZE 256

static void init_lightning (void);
static void finish_lightning (void);

static jit_state_t *_jit;
static void init_trampoline (void);
static void finish_trampoline (void);

static lt_dlhandle dynamic_module;
static void open_dynamic_module ();
static void close_dynamic_module ();

int (*trampoline) (Vm *vm, void *f, void *arg);

void
init_compiler (void)
{
  init_opcode ();
  init_lightning ();
  init_trampoline ();
  open_dynamic_module ();
}

void
finish_compiler (void)
{
  finish_trampoline ();
  finish_lightning ();
  finish_opcode ();
  close_dynamic_module ();
}

Object
compile (Heap *heap, Object code)
{
  Compiler compiler;
  compiler_init (&compiler);

  parse_code (&compiler, code);

  compiler_destroy (&compiler);
}

static void
init_lightning (void)
{
  jit_set_memory_functions (xmalloc, xrealloc, free);
  init_jit (program_name);
}

static void
finish_lightning (void)
{
  finish_jit (void);
}

static void
init_trampoline (void)
{
  _jit = jit_new_state ();

  jit_prolog ();
  jit_frame (FRAME_SIZE);
  jit_node_t *vm = jit_arg ();
  jit_node_t *f = jit_arg ();
  jit_node_t *arg = jit_arg ();
  jit_getarg (JIT_R0, f);
  jit_getarg (JIT_V0, arg);
  jit_jmpr (JIT_R0);

  jit_node_t *done = jit_indirect ();
  jit_retr (JIT_V0);
  jit_epilog ();

  trampoline = jit_emit ();
  trampoline_return = jit_address (done);
  jit_clear_state();
}

static void
finish_trampoline (void)
{
  jit_destroy_state ();
}

static void
init_module (void)
{
  module = lt_dlopen (NULL);
}

static void
finish_module (void)
{
  lt_dlclose (module);
}
