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

    // Inicjalizacja rozmiarów partycji
    printf("$$$ Inicjalizacja rozmiarów partycji...\n");
    for (int i = 0; i < coarseVertices; i++) {
        for (int j = 0; j < fineCounts[i]; j++) {
            int fineVertex = fineToCoarseMap[i][j];
            partition[fineVertex] = partition[i];
            printf("--- Przypisanie wierzchołka coarse %d -> wierzchołek fine %d -> Partycja %d\n", i, fineVertex, partition[i]);
        }
    }

    // Inicjalizacja rozmiarów partycji
    for (int i = 0; i < n; i++) {
        partSizes[partition[i]]++;
    }

    printf("$$$ Początkowe rozmiary partycji:\n");
    for (int i = 0; i < k; i++) {
        printf("--- Partycja %d: %d\n", i, partSizes[i]);
    }

    int maxIterations = 5;
    int iter = 0;

    while (iter++ < maxIterations) {
        int anyImprovement = 0;  // Śledzenie, czy w tej iteracji dokonano jakiejkolwiek poprawy
        printf("$$$ Iteracja %d\n", iter);

        // Iteracja przez każdy wierzchołek w celu poprawy partycji
        for (int v = 0; v < n; v++) {
            int currentPart = partition[v];
            int internal = 0;

            // Resetowanie zewnętrznych krawędzi dla tego wierzchołka
            memset(external, 0, k * sizeof(int));

            // Sprawdzenie wszystkich sąsiadów wierzchołka v i obliczenie krawędzi wewnętrznych i zewnętrznych
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

            // Próba znalezienia najlepszej partycji do przeniesienia wierzchołka
            printf("--- Wierzchołek %d: obecna partycja = %d, wewnętrzne = %d, zewnętrzne = {", v, currentPart, internal);
            for (int p = 0; p < k; p++) {
                printf("%d:%d ", p, external[p]);
            }
            printf("}\n");

            for (int p = 0; p < k; p++) {
                if (p == currentPart || partSizes[p] >= maxSize) continue;

                int gain = external[p] - internal;
                if (gain > bestGain) {
                    bestGain = gain;
                    bestTargetPart = p;
                }
            }

            if (bestTargetPart != -1) {
                printf("--- Najlepszy ruch dla wierzchołka %d to partycja %d z zyskiem %d\n", v, bestTargetPart, bestGain);
            } else {
                printf("--- Nie znaleziono lepszej partycji dla wierzchołka %d\n", v);
            }

            // Jeśli przeniesienie wierzchołka poprawia partycję, wykonaj ruch
            if (bestTargetPart != -1 && partSizes[currentPart] > minSize) {
                printf("--- Przenoszenie wierzchołka %d z partycji %d do partycji %d\n", v, currentPart, bestTargetPart);
                for (int i = 0; i < fineCounts[v]; i++) {
                    if (fineToCoarseMap[v] != NULL && i < fineCounts[v]) {
                        int fineVertex = fineToCoarseMap[v][i];
                        if (partition[fineVertex] == currentPart) {
                            partition[fineVertex] = bestTargetPart;
                            partSizes[currentPart]--;
                            partSizes[bestTargetPart]++;
                            printf("--- Przeniesiono wierzchołek fine %d do partycji %d\n", fineVertex, bestTargetPart);
                        }
                    }
                }
                anyImprovement = 1;
            }
        }

        // Jeśli nie dokonano żadnej poprawy, zakończ wcześniej
        if (!anyImprovement) {
            printf("--- Brak poprawy w iteracji %d, wcześniejsze zakończenie.\n", iter);
            break;
        }
    }

    // Logowanie końcowych rozmiarów partycji
    printf("--- Końcowe rozmiary partycji: ");
    for (int i = 0; i < k; i++) {
        printf("--- Partycja %d: %d ", i, partSizes[i]);
    }
    printf("\n");

    free(partSizes);
    free(external);
}
