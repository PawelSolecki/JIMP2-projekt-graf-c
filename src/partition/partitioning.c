#include "partitioning.h"

int *initialPartition(Graph *coarseGraph, int k) {
    int n = coarseGraph->numVertices;
    int *partition = malloc(n * sizeof(int));
    if (!partition) {
        perror("Nie udało się zaalokować pamięci dla grafu");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        partition[i] = i % k;
    }

    return partition;
}

void partitioning() {
    printf("Partitioning function called.\n");
}
