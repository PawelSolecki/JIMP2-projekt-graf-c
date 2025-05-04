#include "file_writer.h"
#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Struktura pomocnicza do przechowywania informacji o wierzchołku i jego pozycji
typedef struct {
    int index;  // Indeks wierzchołka w grafie
    int row;    // Pozycja wiersza
    int col;    // Pozycja kolumny
} VertexInfo;

// Funkcja porównująca do sortowania wierzchołków według pozycji (najpierw wiersze, potem kolumny)
static int compare_vertex_positions(const void *a, const void *b) {
    const VertexInfo *va = (const VertexInfo *)a;
    const VertexInfo *vb = (const VertexInfo *)b;
    
    if (va->row != vb->row) {
        return va->row - vb->row;
    }
    return va->col - vb->col;
}

// Funkcja porównująca do sortowania zwykłych intów (dla qsort)
static int compare_ints(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Funkcja zapisująca tablicę intów do pliku, oddzielając elementy średnikami
static void write_int_array(FILE *file, const int *array, int count) {
    if (!file || !array || count <= 0) return;
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%d", array[i]);
        if (i < count - 1) {
            fprintf(file, ";");
        }
    }
    fprintf(file, "\n");
}

// Funkcja do zapisania grafu w formacie CSRRG
int save_graph(const Graph *graph, const char *filename) {
    if (!graph || !filename) {
        fprintf(stderr, "Błąd: Nieprawidłowe argumenty dla funkcji save_graph.\n");
        return -1;
    }
    
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Błąd otwarcia pliku do zapisu");
        return -1;
    }
    
    // --- Zbierz informacje o wszystkich wierzchołkach i ich pozycjach ---
    VertexInfo *vertices = malloc(graph->numVertices * sizeof(VertexInfo));
    if (!vertices) {
        perror("Błąd alokacji pamięci dla informacji o wierzchołkach");
        fclose(file);
        return -1;
    }
    
    // Inicjalizuj domyślne wartości
    for (int i = 0; i < graph->numVertices; i++) {
        vertices[i].index = i;
        vertices[i].row = -1;
        vertices[i].col = -1;
    }
    
    // Znajdź pozycje wszystkich wierzchołków
    for (int v = 0; v < graph->numVertices; v++) {
        Node *temp = graph->adjLists[v];
        while (temp) {
            // Zapisz pozycję wierzchołka v, jeśli jest w liście sąsiadów jako pętla
            if (temp->vertex == v) {
                vertices[v].row = temp->row;
                vertices[v].col = temp->column;
                break;
            }
            temp = temp->next;
        }
    }
    
    // Jeśli pozycja nie została znaleziona w pętli, szukaj w listach sąsiadów innych wierzchołków
    for (int v = 0; v < graph->numVertices; v++) {
        if (vertices[v].row == -1 || vertices[v].col == -1) {
            for (int i = 0; i < graph->numVertices; i++) {
                if (i == v) continue; // Pomiń własną listę, już sprawdziliśmy
                
                Node *temp = graph->adjLists[i];
                while (temp) {
                    if (temp->vertex == v) {
                        vertices[v].row = temp->row;
                        vertices[v].col = temp->column;
                        break;
                    }
                    temp = temp->next;
                }
                if (vertices[v].row != -1 && vertices[v].col != -1) {
                    break;
                }
            }
        }
    }
    
    // Sprawdź czy wszystkie pozycje zostały znalezione
    for (int i = 0; i < graph->numVertices; i++) {
        if (vertices[i].row == -1 || vertices[i].col == -1) {
            fprintf(stderr, "Błąd: Nie udało się określić pozycji dla wierzchołka %d.\n", i);
            free(vertices);
            fclose(file);
            return -1;
        }
    }
    
    // Sortuj wierzchołki według pozycji (najpierw według rzędu, potem kolumny)
    qsort(vertices, graph->numVertices, sizeof(VertexInfo), compare_vertex_positions);
    
    // --- Linia 1: Liczba kolumn macierzy ---
    fprintf(file, "%d\n", graph->numCols);
    
    // --- Linie 2 i 3: Indeksy węzłów i wskaźniki wierszy ---
    
    // Znajdź maksymalny numer wiersza
    int max_row = -1;
    for (int i = 0; i < graph->numVertices; i++) {
        if (vertices[i].row > max_row) {
            max_row = vertices[i].row;
        }
    }
    
    // Alokuj pamięć dla tablic
    int *indices_array = malloc(graph->numVertices * sizeof(int));
    int *rowptr_array = malloc((max_row + 2) * sizeof(int));
    
    if (!indices_array || !rowptr_array) {
        perror("Błąd alokacji pamięci dla tablic indeksów");
        free(vertices);
        if (indices_array) free(indices_array);
        if (rowptr_array) free(rowptr_array);
        fclose(file);
        return -1;
    }
    
    // Wypełnij tablicę indices_array
    int indices_idx = 0;
    for (int i = 0; i < graph->numVertices; i++) {
        indices_array[indices_idx++] = vertices[i].col;
    }
    
    // Zbuduj rowptr_array z uwzględnieniem pustych wierszy
    int vertex_idx = 0;
    rowptr_array[0] = 0; // Pierwszy element zawsze 0
    
    for (int row = 0; row <= max_row; row++) {
        // Zlicz wierzchołki w bieżącym wierszu
        int vertices_in_row = 0;
        while (vertex_idx + vertices_in_row < graph->numVertices && 
               vertices[vertex_idx + vertices_in_row].row == row) {
            vertices_in_row++;
        }
        
        // Ustaw wskaźnik dla następnego wiersza
        rowptr_array[row + 1] = rowptr_array[row] + vertices_in_row;
        
        // Przesuń indeks do następnego wiersza
        vertex_idx += vertices_in_row;
    }
    
    // --- Linia 4 i 5: Grupy połączeń i wskaźniki grup ---
    // Najpierw tworzymy tablicę mapującą: indeks wierzchołka w posortowanej tablicy -> oryginalny indeks w grafie
    int *orig_to_sorted = malloc(graph->numVertices * sizeof(int));
    if (!orig_to_sorted) {
        perror("Błąd alokacji pamięci dla tablicy mapującej indeksy");
        free(vertices);
        free(indices_array);
        free(rowptr_array);
        fclose(file);
        return -1;
    }
    
    // Wypełnij tablicę mapującą
    for (int i = 0; i < graph->numVertices; i++) {
        orig_to_sorted[vertices[i].index] = i;
    }
    
    // Przydziel pamięć na grupy - maksymalnie każdy wierzchołek może mieć tyle sąsiadów ile jest wierzchołków
    int *temp_neighbors = malloc(graph->numVertices * sizeof(int)); // Tymczasowa tablica na sąsiadów
    int *groups_array = malloc(graph->numVertices * graph->numVertices * sizeof(int)); // Maksymalna potrzebna pamięć
    int *groupptr_array = malloc((graph->numVertices + 1) * sizeof(int));
    
    if (!temp_neighbors || !groups_array || !groupptr_array) {
        perror("Błąd alokacji pamięci dla grup");
        free(vertices);
        free(indices_array);
        free(rowptr_array);
        free(orig_to_sorted);
        if (temp_neighbors) free(temp_neighbors);
        if (groups_array) free(groups_array);
        if (groupptr_array) free(groupptr_array);
        fclose(file);
        return -1;
    }
    
    // Wypełnij tablice grup z sortowaniem sąsiadów
    int groups_idx = 0;
    int groupptr_idx = 0;
    
    for (int v = 0; v < graph->numVertices; v++) {
        if (graph->adjLists[v] == NULL) continue; // Pomiń izolowane wierzchołki
        
        // Zbierz tylko sąsiadów o numerach większych niż bieżący wierzchołek
        int num_neighbors = 0;
        Node *temp = graph->adjLists[v];
        
        while (temp) {
            // Sprawdź czy to jest połączenie "w przód" (z wyższym numerem wierzchołka)
            if (temp->vertex > v) {
                int sorted_neighbor = orig_to_sorted[temp->vertex];
                temp_neighbors[num_neighbors++] = sorted_neighbor;
            }
            temp = temp->next;
        }
        
        // Jeśli wierzchołek nie ma połączeń "w przód", pomijamy go całkowicie
        if (num_neighbors == 0) {
            continue; // Przejdź do następnego wierzchołka bez tworzenia relacji
        }
        
        // Zapisz wskaźnik na początek grupy
        groupptr_array[groupptr_idx++] = groups_idx;
        
        // Dodaj wierzchołek źródłowy jako pierwszy element grupy
        int sorted_v = orig_to_sorted[v];
        groups_array[groups_idx++] = sorted_v;
        
        // Sortuj listę sąsiadów rosnąco
        qsort(temp_neighbors, num_neighbors, sizeof(int), compare_ints);
        
        // Dodaj posortowanych sąsiadów do groups_array
        for (int i = 0; i < num_neighbors; i++) {
            groups_array[groups_idx++] = temp_neighbors[i];
        }
    }
    groupptr_array[groupptr_idx] = groups_idx; // Końcowy wskaźnik
    
    // Zapisz wszystkie tablice do pliku
    write_int_array(file, indices_array, indices_idx);
    write_int_array(file, rowptr_array, max_row + 2);
    write_int_array(file, groups_array, groups_idx);
    write_int_array(file, groupptr_array, groupptr_idx + 1);
    
    // Zwolnij zaalokowaną pamięć
    free(vertices);
    free(indices_array);
    free(rowptr_array);
    free(orig_to_sorted);
    free(temp_neighbors);
    free(groups_array);
    free(groupptr_array);
    
    fclose(file);
    return 0;
}