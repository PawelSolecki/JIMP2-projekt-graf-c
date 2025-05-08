#include "coarsening.h"
#include "../graph.h"
#include "partition-utils.h"
#include <string.h>

int* matchVertices(Graph* graph) {
    int n = graph->numVertices;
    int* matched = malloc(n * sizeof(int));
    char* visited = calloc(n, sizeof(char));

    for (int i = 0; i < n; i++)
        matched[i] = i;

    for (int v = 0; v < n; v++) {
        if (visited[v]) continue;

        int bestNeighbor = -1;
        int maxDegree = -1;

        for (Node* nbr = graph->adjLists[v]; nbr; nbr = nbr->next) {
            int u = nbr->vertex;
            if (visited[u]) continue;

            // Degree of u
            int degree = 0;
            for (Node* p = graph->adjLists[u]; p; p = p->next)
                degree++;

            if (degree > maxDegree) {
                maxDegree = degree;
                bestNeighbor = u;
            }
        }

        if (bestNeighbor != -1) {
            matched[v] = bestNeighbor;
            matched[bestNeighbor] = v;
            visited[v] = visited[bestNeighbor] = 1;
        } else {
            visited[v] = 1;
        }
    }

    free(visited);
    return matched;
}


Graph* contractGraph(
    Graph* graph,
    int* match,
    DynamicIntList** prevMap,
    DynamicIntList*** newMapOut,
    int* newSizeOut
) {
    int n = graph->numVertices;
    int* superIndex = malloc(n * sizeof(int));
    if (superIndex == NULL) {
        fprintf(stderr, "Failed to allocate memory for superIndex\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) superIndex[i] = -1;

    int superCount = 0;

    // Assign super-node indices
    for (int i = 0; i < n; i++) {
        if (i <= match[i]) {
            if (superIndex[i] == -1) {
                superIndex[i] = superCount;
                if (match[i] != i)
                    superIndex[match[i]] = superCount;
                superCount++;
            }
        }
    }

    // Assign unmatched vertices their own super-node
    for (int i = 0; i < n; i++) {
        if (superIndex[i] == -1) {
            superIndex[i] = superCount++;
        }
    }

    printf("--- Super-node count: %d --- \n", superCount);

    // Initialize new coarseToFine map
    DynamicIntList** newMap = malloc(superCount * sizeof(DynamicIntList*));
    if (newMap == NULL) {
        fprintf(stderr, "Failed to allocate memory for newMap\n");
        free(superIndex);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < superCount; i++) {
        newMap[i] = createDynamicList();
    }

    // Populate new map
    for (int i = 0; i < n; i++) {
        int coarseID = superIndex[i];
        if (coarseID == -1) continue;  // safety check

        if (prevMap == NULL) {
            addToDynamicList(newMap[coarseID], i);
        } else {
            DynamicIntList* fineList = prevMap[i];
            for (int j = 0; j < fineList->size; j++) {
                addToDynamicList(newMap[coarseID], fineList->data[j]);
            }
        }
    }

    // Create new graph with same number of columns
    Graph* newGraph = createGraph(superCount);
    newGraph->numCols = graph->numCols;

    // Edge building with seen table to avoid duplicates
    char* seen = calloc(superCount, sizeof(char));
    if (seen == NULL) {
        fprintf(stderr, "Failed to allocate memory for seen\n");
        free(superIndex);
        for (int i = 0; i < superCount; i++) {
            freeDynamicList(newMap[i]);
        }
        free(newMap);
        exit(EXIT_FAILURE);
    }

    printf("--- New graph size: %d ---\n", superCount);

    for (int v = 0; v < n; v++) {
        int sv = superIndex[v];
        if (sv < 0 || sv >= superCount || newMap[sv] == NULL || newMap[sv]->size == 0) continue;

        printf("--- Processing vertex %d (super-node %d) ---\n", v, sv);
    
        for (Node* nbr = graph->adjLists[v]; nbr != NULL; nbr = nbr->next) {
            int u = nbr->vertex;
        
            // Validate u
            if (u < 0 || u >= n) continue;
        
            int su = superIndex[u];
        
            // Skip if same super node, already seen, or out of bounds
            if (su < 0 || su >= superCount || sv == su || seen[su]) continue;
        
            // Validate both super-nodes exist and have members
            if (!newMap[su] || newMap[su]->size == 0) continue;
            if (!newMap[sv] || newMap[sv]->size == 0) continue;
        
            int fineSrc = newMap[sv]->data[0];
            int fineDest = newMap[su]->data[0];
        
            // Double-check fine vertex IDs are within range
            if (fineSrc < 0 || fineSrc >= graph->numVertices) continue;
            if (fineDest < 0 || fineDest >= graph->numVertices) continue;
        
            Node* srcList = graph->adjLists[fineSrc];
            Node* destList = graph->adjLists[fineDest];
        
            // Be safe: check adjList pointers before accessing fields
            int src_row = srcList ? srcList->row : 0;
            int src_col = srcList ? srcList->column : 0;
            int dest_row = destList ? destList->row : 0;
            int dest_col = destList ? destList->column : 0;
        
            addEdge(newGraph, sv, src_row, src_col, su, dest_row, dest_col);
            seen[su] = 1;
        }
        

        printf("--- Added edges for super-node %d ---\n", sv);
    
        // Reset seen[] safely
        for (Node* nbr = graph->adjLists[v]; nbr != NULL; nbr = nbr->next) {
            int su_reset = superIndex[nbr->vertex];
            if (su_reset >= 0 && su_reset < superCount)
                seen[su_reset] = 0;
        }
    }
    
    printf("--- New graph ---\n");

    free(seen);
    free(superIndex);

    *newMapOut = newMap;
    *newSizeOut = superCount;

    return newGraph;
}


Graph* coarsenGraph(Graph* original, int targetSize, int*** coarseToFineMap, int** fineCounts) {
    Graph* current = cloneGraph(original);
    int currentSize = current->numVertices;

    DynamicIntList** currentMap = NULL;
    
    while (current->numVertices > targetSize) {
        int* matched = matchVertices(current);

        DynamicIntList** newMap = NULL;
        int newSize = 0;

        printf("--- Coarsening step ---\n");

        Graph* next = contractGraph(current, matched, currentMap, &newMap, &newSize);

        printf("Coarsened graph size: %d\n", newSize);

        freeGraph(current);
        free(matched);

        if (currentMap) {
            for (int i = 0; i < currentSize; i++) {
                freeDynamicList(currentMap[i]);
            }
            free(currentMap);
        }

        current = next;
        currentMap = newMap;
        currentSize = newSize;
    }

    // Convert currentMap to final coarseToFineMap and fineCounts
    int** map = malloc(currentSize * sizeof(int*));
    int* counts = malloc(currentSize * sizeof(int));
    memset(counts, -1, currentSize * sizeof(int));

    for (int i = 0; i < currentSize; i++) {
        int size = currentMap[i]->size;
        counts[i] = size;
        map[i] = malloc(size * sizeof(int));
        memset(map[i], -1, size * sizeof(int));
        for (int j = 0; j < size; j++) {
            map[i][j] = currentMap[i]->data[j];
        }
        freeDynamicList(currentMap[i]);
    }

    free(currentMap);

    *coarseToFineMap = map;
    *fineCounts = counts;

    return current;
}
