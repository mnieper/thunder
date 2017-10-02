/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * This program is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <stddef.h>

#include "graph.h"
#include "macros.h"
#include "obstack.h"
#include "xalloc.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

struct my_node
{
  int value;
  Node nodes;
};
typedef struct my_node MyNode;

struct my_edge
{
  char name;
  Edge edges;
};
typedef struct my_edge MyEdge;

#define MY_GRAPH GRAPH (MyNode, nodes, MyEdge, edges)

static struct obstack stack;

void splitter (MyEdge *e, MyNode **n, MyEdge **e1,
	       MyEdge **e2, int *count)
{
  *n = obstack_alloc (&stack, sizeof (MyNode));
  (*n)->value = 10;

  *e1 = obstack_alloc (&stack, sizeof (MyEdge));
  (*e1)->name = '1';

  *e2 = obstack_alloc (&stack, sizeof (MyEdge));
  (*e2)->name = '2';

  ++*count;
}

int
main (int argc, char *argv)
{
  obstack_init (&stack);

  Graph graph;

  MyNode
    n0 = { .value = 0 },
    n1 = { .value = 1 },
    n2 = { .value = 2 },
    n3 = { .value = 3 },
    n4 = { .value = 4 };

  MyEdge
    e0 = { .name = 'Z' },
    e1 = { .name = 'A' },
    e2 = { .name = 'B' },
    e3 = { .name = 'C' },
    e4 = { .name = 'D' },
    e5 = { .name = 'E' },
    e6 = { .name = 'F' };

  graph_init (MY_GRAPH, &graph);

  graph_add_node (MY_GRAPH, &graph, &n0);
  graph_add_node (MY_GRAPH, &graph, &n1);
  graph_add_node (MY_GRAPH, &graph, &n2);
  graph_add_node (MY_GRAPH, &graph, &n3);

  {
    int i = 0;
    node_foreach (n, MY_GRAPH, &graph)
      i += n->value;
    ASSERT (i == 6);
  }

  graph_add_edge (MY_GRAPH, &e0, &n0, &n3);
  graph_add_edge (MY_GRAPH, &e1, &n1, &n2);
  graph_add_edge (MY_GRAPH, &e2, &n1, &n3);
  graph_add_edge (MY_GRAPH, &e3, &n2, &n2);

  ASSERT (edge_critical (MY_GRAPH, &e1));
  ASSERT (!edge_critical (MY_GRAPH, &e0));

  postorder_init (MY_GRAPH, &graph);
  {
    int order[] =  { 0, 1, 2, 3 };
    size_t i = 0;
    reverse_postorder_foreach (n, MY_GRAPH, &graph)
      ASSERT (n->value == order[i++]);
  }

  graph_dominance_init (MY_GRAPH, &graph);

  ASSERT (&n1 == graph_immediate_dominator (MY_GRAPH, &graph, &n2));
  ASSERT (NULL == graph_immediate_dominator (MY_GRAPH, &graph, &n3));

  ASSERT (graph_dominates (MY_GRAPH, &graph, &n1, &n2));
  ASSERT (!graph_dominates (MY_GRAPH, &graph, &n1, &n3));
  ASSERT (graph_dominates (MY_GRAPH, &graph, &n1, &n1));
  ASSERT (!graph_dominates (MY_GRAPH, &graph, &n2, &n3));

  ASSERT (!node_loop_header (MY_GRAPH, &n1));
  ASSERT (node_loop_header (MY_GRAPH, &n2));

  int count = 0;
  graph_split_critical_edges (MY_GRAPH, &graph,
			      (EDGE_SPLITTER (MY_GRAPH)) splitter,
			      &count);

  ASSERT (count == 2);

  graph_destroy (MY_GRAPH, &graph);

  graph_init (MY_GRAPH, &graph);

  graph_add_node (MY_GRAPH, &graph, &n0);
  graph_add_node (MY_GRAPH, &graph, &n1);
  graph_add_node (MY_GRAPH, &graph, &n2);
  graph_add_node (MY_GRAPH, &graph, &n3);
  graph_add_edge (MY_GRAPH, &e0, &n0, &n1);
  graph_add_edge (MY_GRAPH, &e1, &n1, &n0);
  graph_add_edge (MY_GRAPH, &e2, &n0, &n2);
  graph_add_edge (MY_GRAPH, &e3, &n2, &n3);
  graph_add_edge (MY_GRAPH, &e4, &n3, &n3);
  graph_add_edge (MY_GRAPH, &e5, &n3, &n0);
  graph_add_node (MY_GRAPH, &graph, &n4);
  graph_add_edge (MY_GRAPH, &e6, &n4, &n0);

  postorder_init (MY_GRAPH, &graph);

  graph_dominance_init (MY_GRAPH, &graph);

  ASSERT (node_loop_header (MY_GRAPH, &n4) == NULL);
  ASSERT (node_loop_header (MY_GRAPH, &n0) == &n0);
  ASSERT (node_loop_header (MY_GRAPH, &n1) == &n0);
  ASSERT (node_loop_header (MY_GRAPH, &n2) == &n0);
  ASSERT (node_loop_header (MY_GRAPH, &n3) == &n3);

  graph_destroy (MY_GRAPH, &graph);

  obstack_free (&stack, NULL);
}
