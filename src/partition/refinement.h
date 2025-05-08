#ifndef REFINEMENT_H
#define REFINEMENT_H

#include "../graph.h"

/** 
  Dla każdego wierzchołka v w pełnym grafie G
    • Sprawdź, czy przeniesienie v do innej części zmniejsza liczbę przeciętych krawędzi.
    • Jeśli przeniesienie poprawia wynik, przenieś v do nowej części.
  Wynik: Przypisanie wierzchołków do k części.
*/
void refinePartition(Graph *original, int coarseVertices, int *partition, int k, int margin, int **fineToCoarseMap, int *fineCounts);

#endif // REFINEMENT_H