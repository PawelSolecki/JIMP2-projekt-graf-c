#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h> // Dodane dla sqrt w funkcjach pomocniczych

// Reprezentuje pojedynczego sąsiada w liście sąsiedztwa (element listy)
typedef struct Node {
    int vertex;     // Indeks wierzchołka (sekwencyjny numer)
    int row;        // Pozycja wiersza wierzchołka w siatce
    int column;     // Pozycja kolumny wierzchołka w siatce
    struct Node* next;
} Node;

// Reprezentuje cały graf jako tablicę list sąsiedztwa
typedef struct Graph {
    int numVertices;
    int numCols;      // Dodane pole: liczba kolumn w siatce
    Node** adjLists;  // tablica wskaźników do list (jedna lista dla każdego wierzchołka)
} Graph;

// Tworzy nowy węzeł listy sąsiedztwa
Node* createNode(int v, int r, int c);

// Tworzy graf o podanej liczbie wierzchołków
Graph* createGraph(int vertices);

// Dodaje krawędź do grafu (nieskierowanego)
void addEdge(Graph* graph, int src, int src_row, int src_col, int dest, int dest_row, int dest_col);

// Wypisuje reprezentację grafu (listy sąsiedztwa)
void printGraph(Graph* graph);

// Zwalnia pamięć zajmowaną przez graf
void freeGraph(Graph* graph);

// --- Funkcje pomocnicze do obliczania pozycji ---

// Oblicza i zwraca pozycję (wiersz, kolumna) wierzchołka o danym indeksie.
int getVertexPosition(const Graph* graph, int vertexIndex, int* row, int* col);

// Oblicza i zwraca indeks wierzchołka na podstawie pozycji (wiersz, kolumna).
int getVertexIndex(const Graph* graph, int row, int col);

// Oblicza i zwraca odległość euklidesową między dwoma wierzchołkami
Graph* refreshGraphWithPartitions(Graph* graph, int* partition);

// Oblicza i zwraca odległość euklidesową między dwoma wierzchołkami
Graph* cloneGraph(Graph* graph);

#endif // GRAPH_H