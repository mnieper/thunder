/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.

 * Thunder is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
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

/* FIXME: The scanner currently assumes that the underlying charset is
ASCII-compatible.  This can be fixed by using UTF-8 codes directly. */

%top {
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <gmp.h>
#include <mpfr.h>
#include <mpc.h>
#include <string.h>

#include "mbchar.h"
#include "reader.h"
#include "localcharset.h"
#include "uniconv.h"
#include "unistr.h"
#include "uniwidth.h"
#include "vmcommon.h"
#include "xalloc.h"
}

%option noyywrap
%option nodefault
%option noyyalloc
%option noyyrealloc
%option reentrant
%option stack
%option interactive
%option bison-bridge
%option bison-locations
%option header-file="scan.h"

%x SC_STRING
%x BINARY OCTAL DECIMAL HEXADECIMAL

%{
#define YY_USER_ACTION							\
   {									\
     yylloc->first_line = yylineno;					\
     yylloc->first_column = yycolumn;					\
     ucs4_t uc;								\
     const uint8_t *s = yytext;						\
     int n;								\
     while (s = u8_next (&uc, s))					\
       {								\
	 switch (uc)							\
	   {								\
	   case '\n':							\
	     yylineno += 1;						\
	     yycolumn = 1;						\
	     break;							\
	   case '\t':							\
	     yycolumn += 8 - yycolumn % 8;				\
	     break;							\
	   default:							\
	     n = uc_width (uc, locale_charset ());				\
	     if (n >= 0)						\
	       yycolumn += n;						\
	   }								\
       }								\
     yylloc->last_line = yylineno;					\
     yylloc->last_column = yylineno;					\
   }

#define YY_INPUT(buf, result, max_size)					\
  {									\
    mbchar_t c;								\
    size_t n = 0;							\
    while (n < max_size							\
	     && (mbf_getc (c, yyextra->in), !mb_iseof (c))		\
	     && !mb_iseq (c, '\n'))					\
      {									\
        size_t len = max_size - n;					\
        uint8_t* s = u8_conv_from_encoding (locale_charset (),		\
                      			    iconveh_question_mark,	\
				            mb_ptr (c),			\
				            mb_len (c),			\
				            NULL, buf + n, &len);	\
       if (s != (uint8_t *) buf + n)					\
          {								\
            mbf_ungetc (c, yyextra->in);                                \
            free (s);							\
            break;							\
          }	 							\
        n += len;							\
      }									\
    if (mb_iseq (c, '\n'))						\
      buf[n++] = '\n';							\
    if (mb_iseof (c) && ferror (yyin))					\
      YY_FATAL_ERROR ("input in flex scanner failed" );			\
    result = n;								\
  }								       

#define CHECK(expr)				\
   do {						\
     if (!(expr))				\
       return INVALID;				\
   } while (false)
   
static void
begin_number (yyscan_t restrict scanner);

static int
end_number (yyscan_t restrict scanner, bool eof);

static void
reset (yyscan_t restrict scanner, bool in_number);

static bool
set_polar (yyscan_t restrict scanner);

static bool
set_exactness_prefix (yyscan_t restrict scanner, bool exact);

static bool
set_radix_prefix (yyscan_t restrict scanner, int radix);

static bool
set_real (yyscan_t restrict scanner);

static bool
set_imag (yyscan_t restrict scanner);

static bool
set_exact_real (yyscan_t restrict scanner);

static bool
set_exact_imag (yyscan_t restrict scanner);

static int
sign (char c);
%}

UTF8-TAIL            ([\x80-\xbf])
UTF8-1               ([\x00-\x7f])
UTF8-2               ([\xc2-\xdf]{UTF8-TAIL})
UTF8-3               (\xe0[\xa0-\xbf]{UTF8-TAIL}|[\xe1-\xec]{UTF8-TAIL}{2}|\xed[\x80-\x9f]{UTF8-TAIL}|[\xee-\xef]{UTF8-TAIL}{2})
UTF8-4               (\xf0[\x90-\xbf]{UTF8-TAIL}{2}|[\xf1-\xf3]{UTF8-TAIL}{3}|\xf4[\x80-\x8f]{UTF8-TAIL}{2})
UTF8-CHAR            ({UTF8-1}|{UTF8-2}|{UTF8-3}|{UTF8-4})
ANY-CHAR             {UTF8-CHAR}|[.\n]

WHITESPACE [ \f\n\t\v]
DELIMITER  {WHITESPACE}|"("|")"|\"|";"

EXPLICIT-SIGN ("+"|"-")
SIGN          {EXPLICIT-SIGN}?

I   (?i:"i")
INF {EXPLICIT-SIGN}(?i:"inf.0")
NAN {EXPLICIT-SIGN}(?i:"nan.0")

DIGIT-2         [0-1]
DIGIT-8         [0-7]
DIGIT-10        [0-9]
DIGIT-16        [0-9A-Fa-f]
UINTEGER-2      {DIGIT-2}+
UINTEGER-8      {DIGIT-8}+
UINTEGER-10     {DIGIT-10}+
UINTEGER-16     {DIGIT-16}+
RATIONAL-2      {UINTEGER-2}|{UINTEGER-2}"/"{UINTEGER-2}
RATIONAL-8      {UINTEGER-8}|{UINTEGER-8}"/"{UINTEGER-8}
RATIONAL-10     {UINTEGER-10}|{UINTEGER-10}"/"{UINTEGER-10}
RATIONAL-16     {UINTEGER-16}|{UINTEGER-16}"/"{UINTEGER-16}
EXPLICIT-SUFFIX "e"{SIGN}{UINTEGER-10}
SUFFIX          {EXPLICIT-SUFFIX}?
DECIMAL-10      {UINTEGER-10}{EXPLICIT-SUFFIX}|"."{UINTEGER-10}{SUFFIX}|{UINTEGER-10}"."{DIGIT-10}*{SUFFIX}

LETTER               [a-zA-Z]
SPECIAL_INITIAL      "!"|"$"|"%"|"&"|"*"|"/"|":"|"<"|"="|">"|"?"|"^"|"_"|"~"
INITIAL              {LETTER}|{SPECIAL_INITIAL}
SPECIAL-SUBSEQUENT   {EXPLICIT-SIGN}|"."
SUBSEQUENT           {INITIAL}|{DIGIT-10}|{SPECIAL-SUBSEQUENT}
PECULIAR-IDENTIFIER  {EXPLICIT-SIGN}|"..."
IDENTIFIER           {INITIAL}{SUBSEQUENT}*|{PECULIAR-IDENTIFIER}

%%
  int radix = yyextra->radix;
  reset (yyscanner, radix != 0);
  switch (radix)
    {
    case 2:
      BEGIN (BINARY);
      break;
    case 8:
      BEGIN (OCTAL);
    case 10:
      BEGIN (DECIMAL);
      break;
    case 16:
      BEGIN (HEXADECIMAL);
      break;
    }

<INITIAL>{
  ",@"   return COMMAAT;
  "#("   return SHARPPAR;
  "("    |
  ")"    |
  "'"    |
  "`"    |
  ","    return yytext[0];

  "."/{DELIMITER} |
  "."             return '.';
  "."/.           return INVALID;

  {IDENTIFIER}/{DELIMITER} |
  {IDENTIFIER}             { 
                             obstack_init (&yylval->stack);
                             obstack_grow (&yylval->stack, yytext, yyleng);                      
                             return IDENTIFIER; 
                           }
  {IDENTIFIER}/.           return INVALID;

  {IDENTIFIER} {
                 obstack_init (&yylval->stack);
		 obstack_grow (&yylval->stack, yytext, yyleng);
		 return IDENTIFIER;
	       }

  "#\\"{UTF8-CHAR}/{DELIMITER} |
  "#\\"{UTF8-CHAR}             {
                                 u8_mbtouc_unsafe (&yylval->character, 
	                          	           yytext + 2, yyleng - 2);
				 return CHARACTER;
                               }

  "#\\"{UTF8-CHAR}/.           return INVALID; // FIXME: Check for alphabetic!

  (?i:"#t")/{DELIMITER} |
  (?i:"#t")             yylval->boolean = true; return BOOLEAN;

  (?i:"#f")/{DELIMITER} |
  (?i:"#f")             yylval->boolean = false; return BOOLEAN;

  (?i:"#t")/. return INVALID;
  (?i:"#f")/. return INVALID;

  (?i:"#\\newline")/{DELIMITER} |
  (?i:"#\\newline")             yylval->character = '\n'; return CHARACTER;
  (?i:"#\\newline")/.           return INVALID;

  (?i:"#\\space")/{DELIMITER} |
  (?i:"#\\space")             yylval->character = ' '; return CHARACTER;
  (?i:"#\\space")/.           return INVALID;

  ";".

  \" obstack_init (&yylval->stack); yy_push_state (SC_STRING, yyscanner);

  {WHITESPACE}

  {ANY-CHAR} return INVALID;
}

<SC_STRING>{
  \"           return STRING;
  {UTF8-CHAR}  obstack_grow (&yylval->stack, yytext, yyleng);
  \\\"         obstack_1grow (&yylval->stack, '"');
  \\\\         obstack_1grow (&yylval->stack, '\\');
  \\{ANY-CHAR} obstack_free (&yylval->stack, NULL); return INVALID;
}

<INITIAL,BINARY,OCTAL,DECIMAL,HEXADECIMAL>{
  {EXPLICIT-SIGN}{I} {
                       mpfr_set_si (yyextra->imag, sign (yytext [0]), MPFR_RNDN); 
		       CHECK (set_imag (yyscanner));
                     }

  (?i:"#e")          CHECK (set_exactness_prefix (yyscanner, true));
  (?i:"#i")          CHECK (set_exactness_prefix (yyscanner, false));
  (?i:"#b")          CHECK (set_radix_prefix (yyscanner, 2)); BEGIN (BINARY);
  (?i:"#o")          CHECK (set_radix_prefix (yyscanner, 8));  BEGIN (OCTAL);
  (?i:"#d")          CHECK (set_radix_prefix (yyscanner, 10)); BEGIN (DECIMAL);
  (?i:"#x")          CHECK (set_radix_prefix (yyscanner, 16)); BEGIN (HEXADECIMAL);
}

<INITIAL,DECIMAL>{
  {SIGN}{RATIONAL-10}             {
                                    mpq_set_str (yyextra->rational, yytext, 10);
				    mpq_canonicalize (yyextra->rational);
				    CHECK (set_exact_real (yyscanner));
				  }
  {EXPLICIT-SIGN}{RATIONAL-10}{I} {
                                    yytext[yyleng - 1] = '\0';
                                    mpq_set_str (yyextra->rational, yytext, 10);
				    mpq_canonicalize (yyextra->rational);
				    CHECK (set_exact_imag (yyscanner));
				  }
  {SIGN}{DECIMAL-10}              { 
				    mpfr_set_str (yyextra->imag, yytext, 10, MPFR_RNDN);
				    CHECK (set_real (yyscanner));
                                  }
  {EXPLICIT-SIGN}{DECIMAL-10}{I}  {
                                    yytext[yyleng - 1] = '\0'; 
				    mpfr_set_str (yyextra->imag, yytext, 10, MPFR_RNDN);
				    CHECK (set_imag (yyscanner));
                                  }
}

<INITIAL,BINARY>{
  {SIGN}{RATIONAL-2}             {
                                   mpq_set_str (yyextra->rational, yytext, 2);
				   mpq_canonicalize (yyextra->rational);
				   CHECK (set_exact_real (yyscanner));
                                 }
  {EXPLICIT-SIGN}{RATIONAL-2}{I} {
                                   yytext[yyleng - 1] = '\0';
				   mpq_set_str (yyextra->rational, yytext, 2);
				   mpq_canonicalize (yyextra->rational); 
				   CHECK (set_exact_imag (yyscanner));
				 }
}
<INITIAL,OCTAL>{
  {SIGN}{RATIONAL-8}             {
                                   mpq_set_str (yyextra->rational, yytext, 8);
				   mpq_canonicalize (yyextra->rational);
				   CHECK (set_exact_real (yyscanner));
                                 }
  {EXPLICIT-SIGN}{RATIONAL-8}{I} {
                                   yytext[yyleng - 1] = '\0';
				   mpq_set_str (yyextra->rational, yytext, 8);
				   mpq_canonicalize (yyextra->rational);
				   CHECK (set_exact_imag (yyscanner));
				 }
}

<INITIAL,HEXADECIMAL>{
  {SIGN}{RATIONAL-16}             {
                                    mpq_set_str (yyextra->rational, yytext, 16);
				    mpq_canonicalize (yyextra->rational);
		 	 	    CHECK (set_exact_real (yyscanner));
                                  }
  {EXPLICIT-SIGN}{RATIONAL-16}{I} {
                                    yytext[yyleng - 1] = '\0';
			 	    mpq_set_str (yyextra->rational, yytext, 16);
				    mpq_canonicalize (yyextra->rational);
				    CHECK (set_exact_imag (yyscanner));
				  }
}

<BINARY,OCTAL,DECIMAL,HEXADECIMAL>{
  "@" CHECK (set_polar (yyscanner));

  <<EOF>>        return end_number (yyscanner, true);
  ""/{DELIMITER} return end_number (yyscanner, false);
  ""/.           | 
  {ANY-CHAR}     return INVALID;
}

%%
static void
begin_number (yyscan_t restrict scanner)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner);
  if (!context->in_number)
    {
      yy_push_state (DECIMAL, scanner);
      context->in_number = true;
    }
}

static int
end_number (yyscan_t restrict scanner, bool eof)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner);
  
  if (!eof && context->radix)
    return INVALID;
  
  if (!context->real_found)
    return INVALID;

  if (!context->radix)
    yy_pop_state (scanner);

  YYSTYPE *restrict lval = yyget_lval (scanner);

  if (context->exact)
    {
      if (context->inexact_prefix)
	{
	  mpc_init2 (lval->inexact_number, 53);
	  mpc_set_q (lval->inexact_number, context->rational, MPC_RNDNN);
	  return INEXACT_NUMBER;
	}
      mpq_init (lval->exact_number);
      mpq_swap (lval->exact_number, context->rational);
      return EXACT_NUMBER;
    }

  if (context->exact_prefix && mpfr_number_p (context->real) 
      && (!context->imag_found || mpfr_zero_p (context->imag)))
    {
      mpq_init (lval->exact_number);
      inexact_to_exact (lval->exact_number, context->real);
      return EXACT_NUMBER;
    }
  
  /* We are now in the case of an inexact complex number. */
  mpc_init2 (lval->inexact_number, 53);

  if (context->polar)
    {
      mpfr_sin_cos (mpc_imagref (lval->inexact_number), mpc_realref (lval->inexact_number), 
		    context->imag, MPFR_RNDN);
      mpc_mul_fr (lval->inexact_number, lval->inexact_number, context->real, MPC_RNDNN);
    }
  else if (context->imag_found)
    {
      mpc_set_fr_fr (lval->inexact_number, context->real, context->imag, MPC_RNDNN);
    }
  else
    {
      mpc_set_fr (lval->inexact_number, context->real, MPC_RNDNN);
    }

  return INEXACT_NUMBER;
}

static void
reset (yyscan_t restrict scanner, bool in_number)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner);
  YYSTYPE *restrict lval = yyget_lval (scanner);
  context->in_number = in_number;
  context->exact_prefix = false;
  context->inexact_prefix = false;
  context->radix_prefix = false;
  context->real_found = false;
  context->imag_found = false;
  context->polar = false;
}

static bool
set_polar (yyscan_t restrict scanner)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner);
  if ((!context->real_found) || context->imag_found || context->polar)
    return false;
  context->polar = true;
  return true;
}

static bool
set_exactness_prefix (yyscan_t restrict scanner, bool exact)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner);
  if (context->exact_prefix || context->inexact_prefix)
    return false;
  if (exact)
    context->exact_prefix = true;
  else
    context->inexact_prefix = true;
  return true;
}

static bool
set_radix_prefix (yyscan_t restrict scanner, int radix)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner);
  if (context->radix_prefix || context->real_found)
    return false;
  context->radix = radix;
  return true;
}

static bool
set_real (yyscan_t restrict scanner)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner); 

  begin_number (scanner);
  
  if (context->real_found && !context->polar)
    return false;

  if (context->polar)
    {
      context->imag_found = true;
      if (context->exact)
	{	  
	  context->exact = false;
	  mpfr_set_q (context->real, context->rational, MPFR_RNDN);
	}
    }
  else
    {
      context->real_found = true;
      context->exact = false;
      mpfr_swap (context->real, context->imag);	  
    }

  return true;
}

static bool
set_imag (yyscan_t restrict scanner)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner); 

  begin_number (scanner);

  if (context->imag_found || context->polar)
    return false;
  
  context->imag_found = true;
  
  if (!context->real_found)
    {
      mpfr_set_zero (context->real, 1);
      context->real_found = true;
      context->exact = false;
    }
  else if (context->exact)
    {
      context->exact = false;
      mpfr_set_q (context->real, context->rational, MPFR_RNDN);
    }

  return true;
}

static bool
set_exact_real (yyscan_t restrict scanner)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner); 

  begin_number (scanner);
  
  if (context->real_found && !context->polar)
    return false;

  if (context->polar)
    {
      context->imag_found = true;
      if (mpz_sgn (mpq_numref (context->rational)) == 0)
	return true;
      mpfr_set_q (context->imag, context->rational, MPFR_RNDN);
      return set_real (scanner);
    }
  else
    {
      context->real_found = true;
      context->exact = true;
    }

  return true;
}

static bool
set_exact_imag (yyscan_t restrict scanner)
{
  YY_EXTRA_TYPE context = yyget_extra (scanner); 

  begin_number (scanner);

  if (context->imag_found || context->polar)
    return false;

  if (mpz_sgn (mpq_numref (context->rational)) == 0)
    {
      context->imag_found = true;
      return true;
    }

  mpfr_set_q (context->imag, context->rational, MPFR_RNDN);
  return set_imag (scanner);
}

static int
sign (char c)
{
  return c == '+' ? 1 : -1;
}

void *yyalloc (size_t bytes, yyscan_t restrict scanner) {
  return xmalloc (bytes);
}
 
void *yyrealloc (void* ptr, size_t bytes, yyscan_t restrict scanner) {
  return xrealloc (ptr, bytes);
}
