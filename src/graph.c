#include "graph.h"
#include <limits.h> //  INT_MAX
#include <stdio.h> // Dodane dla printf w printGraph

// Tworzy nowy węzeł listy sąsiedztwa
Node* createNode(int v, int r, int c) {
    Node* newNode = malloc(sizeof(Node));
    if (!newNode) {
        perror("Nie udało się zaalokować pamięci dla węzła");
        exit(EXIT_FAILURE);
    }
    newNode->vertex = v;
    newNode->row = r;
    newNode->column = c;
    newNode->next = NULL;
    return newNode;
}

// Tworzy graf o podanej liczbie wierzchołków
Graph* createGraph(int vertices) {
    Graph* graph = malloc(sizeof(Graph));
     if (!graph) {
        perror("Nie udało się zaalokować pamięci dla grafu");
        exit(EXIT_FAILURE);
    }
    graph->numVertices = vertices;
    graph->numCols = 0; // Inicjalizacja numCols - zostanie ustawione później

    // Alokuje pamięć na tablicę list sąsiedztwa
    graph->adjLists = malloc(vertices * sizeof(Node*));
     if (!graph->adjLists) {
        perror("Nie udało się zaalokować pamięci dla list sąsiedztwa");
        free(graph); // Zwolnij wcześniej zaalokowaną pamięć dla grafu
        exit(EXIT_FAILURE);
    }

    // Inicjalizuje każdą listę sąsiedztwa jako pustą (NULL)
    for (int i = 0; i < vertices; i++) {
        graph->adjLists[i] = NULL;
    }

    // Inicjalizuje tablicę do przechowywania przynależności wierzchołków do podziałów
    graph->partitions = calloc(vertices, sizeof(int));

    return graph;
}

// Dodaje krawędź do grafu (nieskierowanego) z pozycjami wierzchołków
void addEdge(Graph* graph, int src, int src_row, int src_col, int dest, int dest_row, int dest_col) {
    // Sprawdzenie poprawności indeksów wierzchołków
    if (!graph || src < 0 || src >= graph->numVertices || dest < 0 || dest >= graph->numVertices) {
        fprintf(stderr, "Błąd: Nieprawidłowe indeksy wierzchołków (%d, %d) dla grafu o %d wierzchołkach.\n",
                src, dest, graph ? graph->numVertices : 0);
        return; // lub inna obsługa błędu
    }

    // Dodaje krawędź z src do dest
    Node* newNodeDest = createNode(dest, dest_row, dest_col);
    newNodeDest->next = graph->adjLists[src];
    graph->adjLists[src] = newNodeDest;

    // Ponieważ graf jest nieskierowany, dodaje również krawędź z dest do src
    Node* newNodeSrc = createNode(src, src_row, src_col);
    newNodeSrc->next = graph->adjLists[dest];
    graph->adjLists[dest] = newNodeSrc;
}

// Wypisuje reprezentację grafu (listy sąsiedztwa), pomijając wierzchołki bez sąsiadów
void printGraph(Graph* graph) {
    if (!graph) {
        printf("Graf jest NULL.\n");
        return;
    }
    printf("Reprezentacja grafu (Listy sąsiedztwa - tylko wierzchołki z sąsiadami):\n");
    printf("Liczba wierzchołków: %d\n", graph->numVertices);
    if (graph->numCols > 0) {
        printf("Liczba kolumn siatki: %d\n", graph->numCols);
    } else {
        printf("Liczba kolumn siatki: (nieustawiona)\n");
    }

    int printed_count = 0; // Licznik wypisanych wierzchołków
    for (int v = 0; v < graph->numVertices; v++) {
        Node* temp = graph->adjLists[v];

        // Pomiń wierzchołki bez sąsiadów (NULL list)
        if (temp == NULL) {
            continue;
        }

        printed_count++;
        // Znajdź pozycję wierzchołka v wśród jego sąsiadów
        int row = -1, col = -1;
        Node* find_pos = graph->adjLists[v];
        while (find_pos) {
            if (find_pos->vertex == v) { // Jeśli wierzchołek ma pętlę do siebie
                row = find_pos->row;
                col = find_pos->column;
                break;
            }
            find_pos = find_pos->next;
        }
        
        // Jeśli nie znaleziono, szukaj w listach innych wierzchołków
        if (row == -1 || col == -1) {
            for (int i = 0; i < graph->numVertices && (row == -1 || col == -1); i++) {
                if (i == v) continue; // Już sprawdziliśmy
                
                Node* search = graph->adjLists[i];
                while (search) {
                    if (search->vertex == v) {
                        row = search->row;
                        col = search->column;
                        break;
                    }
                    search = search->next;
                }
            }
        }
        
        printf("Wierzchołek %d (poz: %d,%d): ", v, row, col);

        // Wypisz sąsiadów
        while (temp) {
            printf("%d(poz:%d,%d)", temp->vertex, temp->row, temp->column);
            temp = temp->next;
            if (temp) { // Dodaj strzałkę tylko jeśli jest kolejny sąsiad
                printf(" -> ");
            }
        }
        printf("\n"); // Zakończ linię po wypisaniu sąsiadów
    }

    if (printed_count == 0) {
        printf("Graf nie zawiera krawędzi (wszystkie listy sąsiedztwa są puste).\n");
    }
}

// Zwalnia pamięć zajmowaną przez graf
void freeGraph(Graph* graph) {
    if (!graph) return;

    if (graph->adjLists) {
        for (int v = 0; v < graph->numVertices; v++) {
            Node* adjList = graph->adjLists[v];
            Node* temp;
            while (adjList != NULL) {
                temp = adjList;
                adjList = adjList->next;
                free(temp);
            }
        }
        free(graph->adjLists);
    }
    free(graph->partitions);
    free(graph);
}

// --- Implementacje funkcji pomocniczych do obliczania pozycji ---

// Oblicza i zwraca pozycję (wiersz, kolumna) wierzchołka o danym indeksie.
int getVertexPosition(const Graph* graph, int vertexIndex, int* row, int* col) {
    if (!graph || !row || !col) {
        fprintf(stderr, "Błąd (getVertexPosition): Wskaźnik na graf, wiersz lub kolumnę jest NULL.\n");
        return -1;
    }
    if (vertexIndex < 0 || vertexIndex >= graph->numVertices) {
        fprintf(stderr, "Błąd (getVertexPosition): Indeks wierzchołka %d jest poza zakresem [0, %d).\n", vertexIndex, graph->numVertices);
        return -1;
    }

    // Szukaj wierzchołka vertexIndex w listach sąsiedztwa
    // Najpierw w jego własnej liście, jeśli ma pętle
    Node* temp = graph->adjLists[vertexIndex];
    while (temp) {
        if (temp->vertex == vertexIndex) {
            *row = temp->row;
            *col = temp->column;
            return 0;
        }
        temp = temp->next;
    }
    
    // Jeśli nie znaleziono, szukaj w listach innych wierzchołków
    for (int v = 0; v < graph->numVertices; v++) {
        if (v == vertexIndex) continue; // Już sprawdziliśmy
        
        temp = graph->adjLists[v];
        while (temp) {
            if (temp->vertex == vertexIndex) {
                *row = temp->row;
                *col = temp->column;
                return 0;
            }
            temp = temp->next;
        }
    }

    fprintf(stderr, "Błąd (getVertexPosition): Nie znaleziono informacji o pozycji dla wierzchołka %d.\n", vertexIndex);
    return -1;
}

// Oblicza i zwraca indeks wierzchołka na podstawie pozycji (wiersz, kolumna).
int getVertexIndex(const Graph* graph, int row, int col) {
    if (!graph) {
        fprintf(stderr, "Błąd (getVertexIndex): Wskaźnik na graf jest NULL.\n");
        return -1;
    }
    
    // Przeszukaj wszystkie listy sąsiedztwa w poszukiwaniu wierzchołka o danej pozycji
    for (int v = 0; v < graph->numVertices; v++) {
        Node* temp = graph->adjLists[v];
        while (temp) {
            if (temp->row == row && temp->column == col) {
                return temp->vertex;
            }
            temp = temp->next;
        }
    }
    
    fprintf(stderr, "Błąd (getVertexIndex): Nie znaleziono wierzchołka o pozycji (%d, %d).\n", row, col);
    return -1;
}

Graph* refreshGraphWithPartitions(Graph* graph) {
    if (!graph || !graph->adjLists || !graph->partitions) {
        fprintf(stderr, "Błąd (applyPartitionsToGraph): Graf jest NULL lub niepoprawny.\n");
        return NULL;
    }

    // Iteracja przez wszystkie wierzchołki
    for (int i = 0; i < graph->numVertices; i++) {
        Node* prev = NULL;
        Node* current = graph->adjLists[i];

        while (current) {
            // Sprawdź, czy sąsiad należy do innego podziału
            if (graph->partitions[i] != graph->partitions[current->vertex]) {
                // Usuń krawędź z listy sąsiedztwa wierzchołka 'i'
                if (prev) {
                    prev->next = current->next;
                } else {
                    graph->adjLists[i] = current->next;  // Jeśli to pierwszy element w liście
                }
                Node* temp = current;
                current = current->next;
                free(temp);  // Usuwamy wierzchołek 'current' z listy sąsiedztwa wierzchołka 'i'

                // Teraz usuwamy krawędź z listy sąsiedztwa wierzchołka 'current->vertex'
                Node* neighborPrev = NULL;
                Node* neighborCurrent = graph->adjLists[current->vertex];
                while (neighborCurrent) {
                    if (neighborCurrent->vertex == i) {
                        if (neighborPrev) {
                            neighborPrev->next = neighborCurrent->next;
                        } else {
                            graph->adjLists[current->vertex] = neighborCurrent->next;
                        }
                        free(neighborCurrent);  // Usuwamy krawędź z listy sąsiedztwa wierzchołka 'current->vertex'
                        break;
                    }
                    neighborPrev = neighborCurrent;
                    neighborCurrent = neighborCurrent->next;
                }
            } else {
                prev = current;
                current = current->next;
            }
        }
    }

    return graph;
}

Graph* cloneGraph(Graph* graph) {
    if (graph == NULL) {
        fprintf(stderr, "Błąd (cloneGraph): Wskaźnik na graf jest NULL.\n");
        return NULL;
    }
    
    Graph* newGraph = createGraph(graph->numVertices);
    if (!newGraph) {
        fprintf(stderr, "Błąd (cloneGraph): Nie udało się utworzyć nowego grafu.\n");
        return NULL;
    }
    newGraph->numCols = graph->numCols; // Skopiuj liczbę kolumn

    for (int i = 0; i < newGraph->numVertices; i++) {
        // Przejdź przez listę oryginalną i skopiuj
        Node* current = graph->adjLists[i];
        Node* prevCopy = NULL;

        while (current) {
            Node* copy = malloc(sizeof(Node));
            copy->vertex = current->vertex;
            copy->row = current->row;
            copy->column = current->column;
            copy->next = NULL;

            if (prevCopy == NULL) {
                newGraph->adjLists[i] = copy;
            } else {
                prevCopy->next = copy;
            }
            prevCopy = copy;
            current = current->next;
        }
    }

    return newGraph;
}