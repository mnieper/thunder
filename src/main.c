/*
 * This file is part of Thunder.
 *
 * Copyright (C) 2017 Marc Nieper-Wißkirchen.
 *
 * Thunder is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <libthunder.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include "closeout.h"
#include "getopt.h"
#include "progname.h"
#include "version-etc.h"

static void print_help (FILE *out);
static void print_version (void);

int main (int argc, char *argv[])
{
  size_t head_size = 1ULL << 30;
  size_t stack_size = 1ULL << 20;
  
  int optc;

  enum {
    OPT_HELP = CHAR_MAX + 1,
    OPT_VERSION
  };

  static struct option longopts[] = {
    { "help",    no_argument, NULL, OPT_HELP },
    { "version", no_argument, NULL, OPT_VERSION },
    { NULL,      0,           NULL, 0 }
  };

  set_program_name (argv[0]);

  setlocale (LC_ALL, "");

  atexit (close_stdout);

  while ((optc = getopt_long (argc, argv, "", longopts, NULL)) != -1)
    switch (optc)
      {
      case OPT_VERSION:
	print_version ();
	exit (EXIT_SUCCESS);
      case OPT_HELP:
	print_help (stdout);
      default:
	print_help (stderr);
      }

  vm_init ();
  Vm *vm = vm_create ();
  vm_free (vm);
}

static void
print_help (FILE *out)
{
  fprintf (out, "Usage: %s [OPTION] ...\n", program_name);
  fputs ("Run the Thunder virtual machine.\n", out);
  fputs ("\n", out);
  fputs ("  --help     display this help and exit\n", out);
  fputs ("  --version  output version information and exit\n", out);
  fputs ("\n", out);
  fprintf (out, "Report bugs to: %s\n", PACKAGE_BUGREPORT);
  fprintf (out, "%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
  
  exit (out == stderr ? EXIT_FAILURE : EXIT_SUCCESS);
}

static void
print_version (void)
{
  version_etc (stdout, NULL, PACKAGE_NAME, VERSION,
	       "Marc Nieper-Wißkirchen", NULL);
}
