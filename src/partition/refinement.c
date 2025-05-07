#include "refinement.h"

void refinePartition(Graph *original, int *partition, int k, int margin, int **fineToCoarseMap) {
    int n = original->numVertices;
    int* partSizes = calloc(k, sizeof(int));
    
    // Oblicz początkowy rozmiar partów
    for (int i = 0; i < n; i++) {
        partSizes[partition[i]]++;
    }

    int targetSize = n / k;
    int maxSize = targetSize + (targetSize * margin / 100);
    
    int improved = 1;
    int maxIterations = 5;
    int iter = 0;

    while (improved && iter++ < maxIterations) {
        improved = 0;

        for (int v = 0; v < n; v++) {
            int currentPart = partition[v];
            int internal = 0;
            int* external = calloc(k, sizeof(int));

            for (Node* nbr = original->adjLists[v]; nbr != NULL; nbr = nbr->next) {
                int p = partition[nbr->vertex];
                if (p == currentPart) internal++;
                else external[p]++;
            }

            for (int p = 0; p < k; p++) {
                if (p == currentPart) continue;
                int gain = external[p] - internal;

                if (gain > 0 && partSizes[p] < maxSize) {
                    // Przeniesienie wierzchołka
                    partition[v] = p;
                    partSizes[currentPart]--;
                    partSizes[p]++;
                    improved = 1;
                    break;
                }
            }

            free(external);
        }
    }

    free(partSizes);
}

