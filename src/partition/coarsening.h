#ifndef COARSENING_H
#define COARSENING_H

#include <stdio.h>
#include "../graph.h"

/**
  1. Utwórz kopię grafu G_c <- G.
  2. Dopóki graf G_c jest zbyt duży:
    • Wybierz niesparowany wierzchołek v.
    • Znajdź sąsiada u maksymalizującego wagę krawędzi w(v, u).
    • Połącz v i u w jeden super-wierzchołek.
    • Zaktualizuj listę sąsiedztwa.
 */
Graph* coarsenGraph(Graph* original, int targetSize, int*** coarseToFineMap, int** fineCounts);

#endif // COARSENING_H