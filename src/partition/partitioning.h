#ifndef PARTITIONING_H
#define PARTITIONING_H

#include <stdio.h>
#include "../graph.h"

/**
  Podziel zredukowany graf G_c k części za pomocą wybranej metody.
    • Podziel zredukowany graf G_c k części.
    • Wykorzystaj jedną z metod - losową, algorytm spektralny lub heurystykę optymalizacyjną.
    • Każdemu wierzchołkowi w G_c przypisz jedną z k części, minimalizując sumę wag przeciętych krawędzi
*/
int *initialPartition(Graph *coarseGraph, int k);

void partitioning();

#endif // PARTITIONING_H