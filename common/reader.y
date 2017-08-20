/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.

 * Thunder is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

%code top {
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
}

%code {
#include "scan.h"
#include "error.h"
}

%code requires {
#include <gmp.h>
#include <mpfr.h>
#include <mpc.h>


#include "mbfile.h"
#include "obstack.h"
#include "unitypes.h"
#include "vmcommon.h"
#include "xalloc.h"

#define YYLTYPE struct location

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free  free

struct elements {
  size_t count;
  struct obstack stack;
};
}

%define api.pure full
%define parse.lac full
%define parse.error verbose
%param {void *scanner}
%locations

%union {
  bool boolean;
  uint32_t character;
  mpq_t exact_number;
  mpc_t inexact_number;
  struct obstack stack;
  struct elements elements;
  Object object;
}

%code provides {
#include <stdbool.h>

// XXX: More union?
struct context
{
  Heap *heap;
  mb_file_t in;
  int radix;
  mpq_t rational;
  mpfr_t real;
  mpfr_t imag;
  union 
  {
    struct
    {
      bool in_number :1;
      bool exact_prefix :1;
      bool inexact_prefix: 1;
      bool radix_prefix :1;
      bool exact :1;
      bool real_found :1;
      bool imag_found :1;
      bool polar :1;
    };
    Object datum;
  };
  Location error_location;
  char *error_message;
};
typedef struct context *restrict Context;
#define YY_EXTRA_TYPE Context

void
yyerror (YYLTYPE *lloc, void *scanner, char const *msg);
}

%token END 0 "end of file"

%token                  SHARPPAR "#("
%token                  INVALID COMMAAT
%token <boolean>        BOOLEAN
%token <character>      CHARACTER
%token <stack>          STRING IDENTIFIER
%token <exact_number>   EXACT_NUMBER
%token <inexact_number> INEXACT_NUMBER

%type <object>   boolean character symbol string vector datum
%type <elements> elements

%destructor { mpq_clear ($$); }                 <exact_number>
%destructor { mpc_clear ($$); }                 <inexact_number>
%destructor { obstack_free (&$$, NULL); }       <stack>
%destructor { obstack_free (&$$.stack, NULL); } <elements>

%%

start: datum
         {
	   yyget_extra (scanner)->datum = $1;
	   YYACCEPT;
	 }
     | %empty
         {
	   yyget_extra (scanner)->datum = make_eof_object ();
	   YYACCEPT;
	 }
     ;

datum: boolean
     | character
     | string
     | symbol
     | vector
     ;

boolean: BOOLEAN
             {
	       $$ = make_boolean ($1);
	     }

character: CHARACTER
             {
	       $$ = make_char ($1);
             }

string: STRING
             {
	       size_t n = obstack_object_size (&$1);
	       uint8_t *s = obstack_finish (&$1);	       
	       size_t len = u8_mbsnlen (s, n);
	       $$ = make_string (yyget_extra (scanner)->heap, len, 0);
	       len *= sizeof (ucs4_t);
	       u8_to_u32 (s, n, string_bytes ($$), &len);
	       obstack_free (&$1, NULL);
	     }

symbol: IDENTIFIER /* XXX: Rename identifier to symbol (?) */
             {
	       size_t n = obstack_object_size (&$1);
	       uint8_t *s = obstack_finish (&$1);
	       $$ = make_symbol (yyget_extra (scanner)->heap, s, n);
	       obstack_free (&$1, NULL);
	     }			 
	       
vector: "#(" elements ')'
          {
	    Object *elements = obstack_finish (&$2.stack);
	    $$ = make_vector (yyget_extra (scanner)->heap, $2.count, 0);
	    for (size_t i = 0; i < $2.count; ++i)
	      vector_set (yyget_extra (scanner)->heap, $$, i, elements[i]);
	    obstack_free (&$2.stack, NULL);
	  }

elements: %empty
            {
	      $$.count = 0;
	      obstack_init (&$$.stack);
	    }
        | elements datum
	    {
	      $1.count++;
	      obstack_ptr_grow (&$1.stack, (void *) $2);
	      $$ = $1;
	    }
%%

void
yyerror (Location *lloc, yyscan_t scanner, char const *msg)
{
  Context context = yyget_extra (scanner);
  context->error_location = *lloc;
  context->error_message = xstrdup (msg);
}

void
context_init (Context context, Heap *heap, int radix, FILE *in)
{
  mpq_init (context->rational);
  mpfr_init (context->real);
  mpfr_init (context->imag);
  context->heap = heap;
  context->radix = radix;
  if (in != NULL)
    mbf_init (context->in, in);
}

void
context_destroy (Context context)
{
  mpq_clear (context->rational);
  mpfr_clear (context->real);
  mpfr_clear (context->imag);
}

Object
read_number (Heap *heap, uint8_t const *bytes, size_t length, int radix)
{
  yyscan_t scanner;
  struct context context;
  context_init (&context, heap, radix, NULL);
  yylex_init_extra (&context, &scanner);
  yyset_debug (1, scanner); // XXX
  YY_BUFFER_STATE buffer = yy_scan_bytes (bytes, length, scanner);
  int res = yyparse (scanner);
  if (res == 2)
    xalloc_die ();
  if (res == 1)
    {
      free (context.error_message);
      context.datum = make_boolean (false);
    }
  yy_delete_buffer (buffer, scanner);
  yylex_destroy (scanner);
  Object datum = context.datum;
  context_destroy (&context);
  return datum;
}

void
reader_init (Reader *restrict reader, Heap *heap, FILE *in)
{
  
  Context context = XMALLOC (struct context);
  context_init (context, heap, 0, in);
  yylex_init_extra (context, &reader->scanner);
  //yyset_debug (1, &reader->scanner); // XXX
  yyset_in (in, reader->scanner);
}

void
reader_destroy (Reader const *restrict reader)
{
  Context context = yyget_extra (reader->scanner);
  context_destroy (context);
  free (context);
  yylex_destroy (reader->scanner);
}

bool
reader_read (Reader const *reader, Object *restrict object, Location *lloc, char **message)
{
  Context context = yyget_extra (reader->scanner);
  int res = yyparse (reader->scanner);
  if (res == 2)
    xalloc_die ();
  if (res == 1)
    {
      if (lloc != NULL)
	*lloc = context->error_location;
      if (message != NULL) {
	*message = context->error_message;
      } else {
	free (context->error_message);
      }
      return false;
    }
  *object = context->datum;
  return true;
}
