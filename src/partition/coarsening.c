#include "coarsening.h"
#include "../graph.h"
#include "partition-utils.h"    

// coarseToFineMap[3] = {5, 8}; - jest po to żeby wiedzieć, które wierzchołki są połączone

int* matchVertices(Graph* graph) {
    int n = graph->numVertices;
    int* matched = malloc(n * sizeof(int));
    char* visited = calloc(n, sizeof(char));

    for (int v = 0; v < n; v++) {
        matched[v] = v;  // domyślnie: nieprzypisany
    }

    for (int v = 0; v < n; v++) {
        if (visited[v]) continue;

        Node* maxNeighbor = NULL;
        int maxDegree = -1;

        for (Node* nbr = graph->adjLists[v]; nbr; nbr = nbr->next) {
            int u = nbr->vertex;
            if (!visited[u]) {
                int degree = 0;
                for (Node* p = graph->adjLists[u]; p; p = p->next) degree++;

                if (degree > maxDegree) {
                    maxDegree = degree;
                    maxNeighbor = nbr;
                }
            }
        }

        if (maxNeighbor) {
            int u = maxNeighbor->vertex;
            matched[v] = u;
            matched[u] = v;
            visited[v] = 1;
            visited[u] = 1;
        } else {
            // brak sąsiadów, nieparowany
            visited[v] = 1;
        }
    }

    free(visited);
    return matched;
}

Graph* contractGraph(Graph* graph, int* match, DynamicIntList** coarseToFineMap) {
    int n = graph->numVertices;
    int* superIndex = malloc(n * sizeof(int));
    int superCount = 0;

    for (int i = 0; i < n; i++) superIndex[i] = -1;

    // Przypisz nowe indeksy superwierzchołków
    for (int i = 0; i < n; i++) {
        if (i <= match[i]) {
            if (superIndex[i] == -1) {
                superIndex[i] = superCount;
                if (match[i] != i) {
                    superIndex[match[i]] = superCount;
                }
                superCount++;
            }
        }
    }

    Graph* newGraph = createGraph(superCount);

    // Zaktualizuj coarseToFineMap
    for (int i = 0; i < n; i++) {
        int coarseID = superIndex[i];
        addToDynamicList(coarseToFineMap[coarseID], i);
    }

    char* seen = calloc(superCount, sizeof(char));

    for (int v = 0; v < n; v++) {
        int sv = superIndex[v];

        for (Node* nbr = graph->adjLists[v]; nbr != NULL; nbr = nbr->next) {
            int u = nbr->vertex;
            int su = superIndex[u];

            if (sv == su) continue;

            if (!seen[su]) {
                addEdge(newGraph, sv, graph->adjLists[v]->row, graph->adjLists[v]->column,
                        su, graph->adjLists[u]->row, graph->adjLists[u]->column);
                seen[su] = 1;
            }
        }

        for (Node* nbr = graph->adjLists[v]; nbr != NULL; nbr = nbr->next)
            seen[superIndex[nbr->vertex]] = 0;
    }

    free(seen);
    free(superIndex);
    return newGraph;
}

Graph* coarsenGraph(Graph* original, int targetSize, int*** coarseToFineMap) {
    Graph* current = cloneGraph(original);
    int currentSize = current->numVertices;

    // Temporary dynamic map
    DynamicIntList** dynamicMap = malloc(currentSize * sizeof(DynamicIntList*));
    for (int i = 0; i < currentSize; i++) {
        dynamicMap[i] = createDynamicList();
    }

    while (current->numVertices > targetSize) {
        int* matched = matchVertices(current);
        Graph* next = contractGraph(current, matched, dynamicMap);

        freeGraph(current);
        free(matched);
        current = next;
        currentSize = current->numVertices;
    }

    // Allocate output 2D array for coarseToFineMap
    int** map = malloc(current->numVertices * sizeof(int*));
    for (int i = 0; i < current->numVertices; i++) {
        int size = dynamicMap[i]->size;
        map[i] = malloc((size + 1) * sizeof(int)); // First value is the size
        map[i][0] = size;
        for (int j = 0; j < size; j++) {
            map[i][j + 1] = dynamicMap[i]->data[j];
        }
    }

    // Clean up dynamic map
    for (int i = 0; i < currentSize; i++) {
        freeDynamicList(dynamicMap[i]);
    }
    free(dynamicMap);

    *coarseToFineMap = map;
    return current;
}


