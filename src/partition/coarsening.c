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

            // Stopień wierzchołka u
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
        fprintf(stderr, "Nie udało się przydzielić pamięci dla superIndex\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) superIndex[i] = -1;

    int superCount = 0;

    // Przypisanie indeksów super-wierzchołków
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

    // Przypisanie niepołączonych wierzchołków do ich własnych super-wierzchołków
    for (int i = 0; i < n; i++) {
        if (superIndex[i] == -1) {
            superIndex[i] = superCount++;
        }
    }

    printf("--- Liczba super-wierzchołków: %d --- \n", superCount);

    // Inicjalizacja nowej mapy coarseToFine
    DynamicIntList** newMap = malloc(superCount * sizeof(DynamicIntList*));
    if (newMap == NULL) {
        fprintf(stderr, "Nie udało się przydzielić pamięci dla newMap\n");
        free(superIndex);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < superCount; i++) {
        newMap[i] = createDynamicList();
    }

    // Wypełnianie nowej mapy
    for (int i = 0; i < n; i++) {
        int coarseID = superIndex[i];
        if (coarseID == -1) continue;  // sprawdzenie bezpieczeństwa

        if (prevMap == NULL) {
            addToDynamicList(newMap[coarseID], i);
        } else {
            DynamicIntList* fineList = prevMap[i];
            for (int j = 0; j < fineList->size; j++) {
                addToDynamicList(newMap[coarseID], fineList->data[j]);
            }
        }
    }

    // Tworzenie nowego grafu z tą samą liczbą kolumn
    Graph* newGraph = createGraph(superCount);
    newGraph->numCols = graph->numCols;

    // Tworzenie krawędzi z użyciem tablicy seen, aby uniknąć duplikatów
    char* seen = calloc(superCount, sizeof(char));
    if (seen == NULL) {
        fprintf(stderr, "Nie udało się przydzielić pamięci dla seen\n");
        free(superIndex);
        for (int i = 0; i < superCount; i++) {
            freeDynamicList(newMap[i]);
        }
        free(newMap);
        exit(EXIT_FAILURE);
    }

    printf("--- Rozmiar nowego grafu: %d ---\n", superCount);

    for (int v = 0; v < n; v++) {
        int sv = superIndex[v];
        if (sv < 0 || sv >= superCount || newMap[sv] == NULL || newMap[sv]->size == 0) continue;

        printf("--- Przetwarzanie wierzchołka %d (super-wierzchołek %d) ---\n", v, sv);
    
        for (Node* nbr = graph->adjLists[v]; nbr != NULL; nbr = nbr->next) {
            int u = nbr->vertex;
        
            // Walidacja u
            if (u < 0 || u >= n) continue;
        
            int su = superIndex[u];
        
            // Pomijanie, jeśli ten sam super-wierzchołek, już widziany lub poza zakresem
            if (su < 0 || su >= superCount || sv == su || seen[su]) continue;
        
            // Walidacja, czy oba super-wierzchołki istnieją i mają członków
            if (!newMap[su] || newMap[su]->size == 0) continue;
            if (!newMap[sv] || newMap[sv]->size == 0) continue;
        
            int fineSrc = newMap[sv]->data[0];
            int fineDest = newMap[su]->data[0];
        
            // Podwójne sprawdzenie, czy ID wierzchołków są w zakresie
            if (fineSrc < 0 || fineSrc >= graph->numVertices) continue;
            if (fineDest < 0 || fineDest >= graph->numVertices) continue;
        
            Node* srcList = graph->adjLists[fineSrc];
            Node* destList = graph->adjLists[fineDest];
        
            // Bezpieczeństwo: sprawdzenie wskaźników adjList przed dostępem do pól
            int src_row = srcList ? srcList->row : 0;
            int src_col = srcList ? srcList->column : 0;
            int dest_row = destList ? destList->row : 0;
            int dest_col = destList ? destList->column : 0;
        
            addEdge(newGraph, sv, src_row, src_col, su, dest_row, dest_col);
            seen[su] = 1;
        }
        

        printf("--- Dodano krawędzie dla super-wierzchołka %d ---\n", sv);
    
        // Resetowanie tablicy seen[] w sposób bezpieczny
        for (Node* nbr = graph->adjLists[v]; nbr != NULL; nbr = nbr->next) {
            int su_reset = superIndex[nbr->vertex];
            if (su_reset >= 0 && su_reset < superCount)
                seen[su_reset] = 0;
        }
    }
    
    printf("--- Nowy graf ---\n");

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

        printf("--- Krok koarseningu ---\n");

        Graph* next = contractGraph(current, matched, currentMap, &newMap, &newSize);

        printf("Rozmiar grafu po koarseningu: %d\n", newSize);

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

    // Konwersja currentMap na końcową coarseToFineMap i fineCounts
    int** map = malloc(original->numVertices * sizeof(int*));
    int* counts = malloc(original->numVertices * sizeof(int));
    memset(counts, -1, original->numVertices * sizeof(int));

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
