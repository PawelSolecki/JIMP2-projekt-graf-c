#include "refinement.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "refinement.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void refinePartition(Graph *original, int coarseVertices, int *partition, int k, int margin, int **fineToCoarseMap, int *fineCounts) {
    if (!original || !partition || k <= 0 || !fineToCoarseMap || !fineCounts) return;

    int n = original->numVertices;
    int* partSizes = calloc(k, sizeof(int));
    int* external = calloc(k, sizeof(int));
    int targetSize = n / k;
    int maxSize = targetSize + (targetSize * margin / 100);
    int minSize = targetSize - (targetSize * margin / 100);

    // Init rozmiarów partycji (partition sizes)
    for (int i = 0; i < coarseVertices; i++) {
        for (int j = 0; j < fineCounts[i]; j++) {
            int fineVertex = fineToCoarseMap[i][j];
            partition[fineVertex] = partition[i];
        }
    }

    for (int i = 0; i < n; i++) {
        partSizes[partition[i]]++;
    }

    int improved = 1;
    int maxIterations = 5;
    int iter = 0;

    while (improved && iter++ < maxIterations) {
        improved = 0;

        // Iterate through each vertex to attempt partition improvement
        for (int v = 0; v < n; v++) {
            int currentPart = partition[v];
            int internal = 0;

            // Reset external edges for this vertex
            memset(external, 0, k * sizeof(int));

            // Check all neighbors of vertex v and calculate external and internal edges
            for (Node* nbr = original->adjLists[v]; nbr != NULL; nbr = nbr->next) {
                int p = partition[nbr->vertex];
                if (p == currentPart) {
                    internal++;
                } else {
                    external[p]++;
                }
            }

            int bestGain = 0;
            int bestTargetPart = -1;

            // Try to find the best partition to move the vertex to
            for (int p = 0; p < k; p++) {
                if (p == currentPart || partSizes[p] >= maxSize) continue;

                int gain = external[p] - internal;
                if (gain > bestGain) {
                    bestGain = gain;
                    bestTargetPart = p;
                }
            }

            // If moving vertex improves the partition, perform the move
            if (bestTargetPart != -1 && partSizes[currentPart] > minSize) {
                // Move fine vertices belonging to coarse vertex v
                for (int i = 0; i < fineCounts[v]; i++) {
                    int fineVertex = fineToCoarseMap[v][i];
                    if (partition[fineVertex] == currentPart) {
                        partition[fineVertex] = bestTargetPart;
                        partSizes[currentPart]--;
                        partSizes[bestTargetPart]++;
                    }
                }
                improved = 1;
            }
        }
    }
    
    free(partSizes);
    free(external);
}

