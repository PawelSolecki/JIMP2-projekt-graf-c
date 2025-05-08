#include "partitioning.h"
#include <time.h>
#include <string.h>

int *initialPartition(Graph *coarseGraph, int uncoarseNumVertices, int k) {
    int n = coarseGraph->numVertices;
    int *partition = malloc(uncoarseNumVertices * sizeof(int));
    if (partition) {
        memset(partition, -1, uncoarseNumVertices * sizeof(int));
    }
    if (!partition) {
        perror("Failed to allocate memory for partition array");
        exit(EXIT_FAILURE);
    }

    int *partSizes = calloc(k, sizeof(int));
    if (!partSizes) {
        perror("Failed to allocate memory for partition sizes");
        free(partition);
        exit(EXIT_FAILURE);
    }

    int targetSize = n / k;

    srand(time(NULL));

    for (int i = 0; i < n; i++) {
        int bestPart = -1;
        int minSize = n + 1;

        for (int j = 0; j < k; j++) {
            if (partSizes[j] < targetSize + 1 && partSizes[j] < minSize) {
                minSize = partSizes[j];
                bestPart = j;
            }
        }

        if (bestPart == -1) bestPart = rand() % k;

        partition[i] = bestPart;
        partSizes[bestPart]++;
    }

    free(partSizes);
    return partition;
}
