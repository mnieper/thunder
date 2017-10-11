/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "common.h"

#define PROGRAM (&(compiler)->program)

/*
 * Overview over the algorithm
 * ===========================
 *
 * We have a prespill-phase. We go through the dominator tree.
 * Whenever register pressure becomes too high, we mark a register to
 * be spilled.  For each subsequent use of the register, we lower the
 * amount of free registers (so that other registers may be spilled).
 *
 * Afterwards, the coloring phase starts.  For unspilled registers,
 * nothing is to be done.  For spilled registers, we make a post-order
 * traversal of the dominance tree.  As long as a register is free,
 * may use that register to reflect the value of a spilled register.
 * We have to load it as soon as that register is needed.
 *
 * Does Post-Order-Traversal work? Or should we think of new
 * life-spans (?)
 * New life-spans are subtrees created during first preorder traversal.
 * When a re-spill occurs, the current sub-tree is shortened... What
 * is the current subtree?
 *
 *
 * Problem: Wenn ein gespilltes Register zurückgeholt wird, brauchen
 * wir ein oder zwei Scratch-Register.  Woher bekommen wir diese? Wir
 * müssen evtl.  weitere Register spillen.  Das zieht dann aber bei
 * vorhergehenden Uses dieser neu gespillten Register den Bedarf eines
 * Scratch-Registers nach sich.  Aber das geht immer, da wir einfach
 * das Register nehmen können, was es vorher inne hatte.  Wie sieht
 * das dann mit späteren Uses aus?  Idee: Wir merken uns die Register
 * im Traversal des Dominance-Trees.  Genaue Datenstruktur???
 */

void program_register_allocate (struct compiler *compiler)
{
  // Für jede Variable erzeuge ein Intervall.  Diese liegen in
  // "unhandled".  Wir gehen diese in Dominance-Tree-Reihenfolge
  // durch.
  // Am Anfang sind alle Variablen auf "not spilled" gesetzt.
  //
  // Sobald ein Intervall bearbeitet wird, kommt es in die "Active
  // Liste".
  //
  // Wenn die Register Pressure zu hoch ist, müssen wir spillen.  Dazu
  // machen wir folgendes: Wir suchen das "active" Intervall mit dem
  // spätesten Endpunkt.  Das fliegt heraus.
  // Wenn wir ein "Use" oder "Def" finden, und es gibt kein Intervall,
  // so erzeugen wir ein neues Intervall in diesem
  // Dominance-Tree-Abschnitt.  Insgesamt gehören zu einer Variablen
  // dann eventuell mehrere Intervalle.  Diese sind disjunkt.  Zu den
  // Intervallen gehört jeweils eine Variable.  Wir müssen auch noch
  // nachhalten, welche Register zu welchem Zeitpunkt besetzt sind.
  //
  // Um die kleinen Bäume zu machen, brauchen wir auch immer
  // "last-use" (wie ist es mit "Last-Def"?) in einem Dominance-Tree.
  // Außerdem brauchen wir die Zahl eines "Spill-Slots".
  //
  // Zu "Definitionen vs. Uses":
  //
  // X <--- Y
  // Y <--- Z
  // In diesem Falle haben X und Z den gleichen Zeitstempel.  Wir
  // dürfen X aber vergessen, denn die Zeit könnte abgelaufen sein.
  // (Es ist "Def".)
  //
  // Wir brauchen Algorithmus: Gegeben Teilbaum des
  // Dominance-Trees. Wann ist die Livetime einer Kongruenzklasse
  // dort?

  program_block_foreach (block, PROGRAM)
    {
      block_instruction_foreach (ins, block)
	{
	  dest_foreach (var, ins)
	    var->spill_slot = -1;
	}
    }

}
