#include "file_writer.h"
#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Struktura pomocnicza do przechowywania informacji o wierzchołku i jego pozycji
typedef struct {
    int index;  // Indeks wierzchołka w grafie
    int row;    // Pozycja wiersza
    int col;    // Pozycja kolumny
} VertexInfo;

// Funkcja porównująca do sortowania wierzchołków według pozycji
static int compare_vertex_positions(const void *a, const void *b) {
    const VertexInfo *va = (const VertexInfo *)a;
    const VertexInfo *vb = (const VertexInfo *)b;
    
    if (va->row != vb->row) {
        return va->row - vb->row;
    }
    return va->col - vb->col;
}

// Funkcja porównująca do sortowania zwykłych intów
static int compare_ints(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// 1. Funkcja zbierająca informacje o wszystkich wierzchołkach i ich pozycjach
static VertexInfo* collect_vertex_info(const Graph *graph, int *max_row_out) {
    if (!graph) return NULL;
    
    VertexInfo *vertices = malloc(graph->numVertices * sizeof(VertexInfo));
    if (!vertices) {
        perror("Błąd alokacji pamięci dla informacji o wierzchołkach");
        return NULL;
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
            if (temp->vertex == v) {
                vertices[v].row = temp->row;
                vertices[v].col = temp->column;
                break;
            }
            temp = temp->next;
        }
    }
    
    // Jeśli pozycja nie została znaleziona w pętli, szukaj w listach innych wierzchołków
    for (int v = 0; v < graph->numVertices; v++) {
        if (vertices[v].row == -1 || vertices[v].col == -1) {
            for (int i = 0; i < graph->numVertices; i++) {
                if (i == v) continue;
                
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
            return NULL;
        }
    }
    
    // Znajdź maksymalny numer wiersza jeśli potrzebny
    if (max_row_out) {
        int max_row = -1;
        for (int i = 0; i < graph->numVertices; i++) {
            if (vertices[i].row > max_row) {
                max_row = vertices[i].row;
            }
        }
        *max_row_out = max_row;
    }
    
    // Sortuj wierzchołki według pozycji
    qsort(vertices, graph->numVertices, sizeof(VertexInfo), compare_vertex_positions);
    
    return vertices;
}

// 2. Funkcja generująca tablicę indeksów (linia 2 formatu CSRRG)
static int* create_indices_array(const VertexInfo *vertices, int vertex_count, int *indices_count_out) {
    if (!vertices || vertex_count <= 0) return NULL;
    
    int *indices_array = malloc(vertex_count * sizeof(int));
    if (!indices_array) {
        perror("Błąd alokacji pamięci dla tablicy indeksów");
        return NULL;
    }
    
    // Wypełnij tablicę indices_array
    int indices_idx = 0;
    for (int i = 0; i < vertex_count; i++) {
        indices_array[indices_idx++] = vertices[i].col;
    }
    
    if (indices_count_out) {
        *indices_count_out = indices_idx;
    }
    
    return indices_array;
}

// 3. Funkcja generująca tablicę wskaźników wierszy (linia 3 formatu CSRRG)
static int* create_rowptr_array(const VertexInfo *vertices, int vertex_count, int max_row, int *rowptr_count_out) {
    if (!vertices || max_row < 0) return NULL;
    
    int *rowptr_array = malloc((max_row + 2) * sizeof(int));
    if (!rowptr_array) {
        perror("Błąd alokacji pamięci dla tablicy wskaźników wierszy");
        return NULL;
    }
    
    // Zbuduj rowptr_array z uwzględnieniem pustych wierszy
    int vertex_idx = 0;
    rowptr_array[0] = 0; // Pierwszy element zawsze 0
    
    for (int row = 0; row <= max_row; row++) {
        // Zlicz wierzchołki w bieżącym wierszu
        int vertices_in_row = 0;
        while (vertex_idx + vertices_in_row < vertex_count && 
               vertices[vertex_idx + vertices_in_row].row == row) {
            vertices_in_row++;
        }
        
        // Ustaw wskaźnik dla następnego wiersza
        rowptr_array[row + 1] = rowptr_array[row] + vertices_in_row;
        
        // Przesuń indeks do następnego wiersza
        vertex_idx += vertices_in_row;
    }
    
    if (rowptr_count_out) {
        *rowptr_count_out = max_row + 2;
    }
    
    return rowptr_array;
}

// 4. Funkcja generująca mapowanie indeksów
static int* create_vertex_mapping(const VertexInfo *vertices, int vertex_count) {
    if (!vertices || vertex_count <= 0) return NULL;
    
    int *orig_to_sorted = malloc(vertex_count * sizeof(int));
    if (!orig_to_sorted) {
        perror("Błąd alokacji pamięci dla tablicy mapującej indeksy");
        return NULL;
    }
    
    // Wypełnij tablicę mapującą
    for (int i = 0; i < vertex_count; i++) {
        orig_to_sorted[vertices[i].index] = i;
    }
    
    return orig_to_sorted;
}

// 5. Funkcja generująca grupy połączeń (linie 4 i 5 formatu CSRRG)
static void create_connection_groups(const Graph *graph, const VertexInfo *vertices, int *vertex_mapping,
                            int **groups_out, int *groups_count,
                            int **groupptr_out, int *groupptr_count) {
    if (!graph || !vertices || !vertex_mapping || !groups_out || !groups_count || !groupptr_out || !groupptr_count) {
        return;
    }
    
    // Przydziel pamięć na grupy
    int *temp_neighbors = malloc(graph->numVertices * sizeof(int));
    int *groups_array = malloc(graph->numVertices * graph->numVertices * sizeof(int));
    int *groupptr_array = malloc((graph->numVertices + 1) * sizeof(int));
    
    if (!temp_neighbors || !groups_array || !groupptr_array) {
        perror("Błąd alokacji pamięci dla grup");
        if (temp_neighbors) free(temp_neighbors);
        if (groups_array) free(groups_array);
        if (groupptr_array) free(groupptr_array);
        return;
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
            // Sprawdź czy to jest połączenie "w przód"
            if (temp->vertex > v) {
                int sorted_neighbor = vertex_mapping[temp->vertex];
                temp_neighbors[num_neighbors++] = sorted_neighbor;
            }
            temp = temp->next;
        }
        
        // Jeśli wierzchołek nie ma połączeń "w przód", pomijamy go
        if (num_neighbors == 0) {
            continue;
        }
        
        // Zapisz wskaźnik na początek grupy
        groupptr_array[groupptr_idx++] = groups_idx;
        
        // Dodaj wierzchołek źródłowy jako pierwszy element grupy
        int sorted_v = vertex_mapping[v];
        groups_array[groups_idx++] = sorted_v;
        
        // Sortuj listę sąsiadów rosnąco
        qsort(temp_neighbors, num_neighbors, sizeof(int), compare_ints);
        
        // Dodaj posortowanych sąsiadów do groups_array
        for (int i = 0; i < num_neighbors; i++) {
            groups_array[groups_idx++] = temp_neighbors[i];
        }
    }
    groupptr_array[groupptr_idx] = groups_idx; // Końcowy wskaźnik
    
    // Ustaw wyniki
    *groups_out = groups_array;
    *groups_count = groups_idx;
    *groupptr_out = groupptr_array;
    *groupptr_count = groupptr_idx + 1;
    
    // Zwolnij tymczasową pamięć
    free(temp_neighbors);
}

// Kodowanie liczby całkowitej jako varint
// Zwraca liczbę zapisanych bajtów
static int write_varint(FILE *file, uint32_t value) {
    int bytes_written = 0;
    uint8_t byte;
    
    do {
        byte = value & 0x7F; // Weź 7 najmniej znaczących bitów
        value >>= 7;         // Przesuń o 7 bitów w prawo
        
        // Jeśli są jeszcze bity do zapisania, ustaw najwyższy bit
        if (value > 0) {
            byte |= 0x80;
        }
        
        fwrite(&byte, 1, 1, file);
        bytes_written++;
    } while (value > 0);
    
    return bytes_written;
}

// Zapisuje tablicę jako varint z kodowaniem delta
static int write_varint_delta(FILE *file, const int *array, int count) {
    int bytes_written = 0;
    
    // Zapisz pierwszy element bez delty
    if (count > 0) {
        bytes_written += write_varint(file, array[0]);
    }
    
    // Zapisz pozostałe elementy jako delty
    for (int i = 1; i < count; i++) {
        int delta = array[i] - array[i-1];
        bytes_written += write_varint(file, delta);
    }
    
    return bytes_written;
}

// Zapisuje tablicę jako varint bez kodowania delta
static int write_varint_array(FILE *file, const int *array, int count) {
    int bytes_written = 0;
    
    for (int i = 0; i < count; i++) {
        bytes_written += write_varint(file, array[i]);
    }
    
    return bytes_written;
}

// Zapisuje dane w formacie little endian (jeśli potrzeba)
static void write_le_uint16(FILE *file, uint16_t value) {
    uint8_t buffer[2];
    buffer[0] = value & 0xFF;         // Najmniej znaczący bajt
    buffer[1] = (value >> 8) & 0xFF;  // Najbardziej znaczący bajt
    fwrite(buffer, 1, 2, file);
}

static void write_le_uint32(FILE *file, uint32_t value) {
    uint8_t buffer[4];
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
    fwrite(buffer, 1, 4, file);
}

// Struktura nagłówka pliku binarnego
typedef struct {
    uint8_t endianness_flag[2];  // Zawsze 'A', 'B'
    uint16_t matrix_size;        // Liczba kolumn macierzy
    uint16_t groupptr_offset;    // Offset do wskaźników grup
    uint32_t indices_offset;     // Offset do indeksów węzłów
    uint32_t groups_offset;      // Offset do grup krawędzi
} BinaryFileHeader;

// Zapisuje nagłówek pliku binarnego
static void write_binary_header(FILE *file, BinaryFileHeader *header) {
    if (!file || !header) return;
    
    // 1. Endianness flag (2 bajty) - zawsze "AB"
    fwrite(header->endianness_flag, 1, 2, file);
    
    // 2. Rozmiar macierzy (2 bajty)
    write_le_uint16(file, header->matrix_size);
    
    // 3. Offset wskaźników grup (2 bajty)
    write_le_uint16(file, header->groupptr_offset);
    
    // 4. Offset indeksów węzłów (4 bajty)
    write_le_uint32(file, header->indices_offset);
    
    // 5. Offset grup krawędzi (4 bajty)
    write_le_uint32(file, header->groups_offset);
}

// Główna funkcja do zapisania grafu w formacie binarnym
int save_graph_bin(const Graph *graph, const char *filename) {
    if (!graph || !filename) {
        fprintf(stderr, "Błąd: Nieprawidłowe argumenty dla funkcji save_graph_binary.\n");
        return -1;
    }
    
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("Błąd otwarcia pliku binarnego do zapisu");
        return -1;
    }
    
    int max_row = 0;
    VertexInfo *vertices = collect_vertex_info(graph, &max_row);
    if (!vertices) {
        fclose(file);
        return -1;
    }
    
    // Przygotuj dane
    int indices_count = 0;
    int *indices_array = create_indices_array(vertices, graph->numVertices, &indices_count);
    if (!indices_array) {
        free(vertices);
        fclose(file);
        return -1;
    }
    
    int rowptr_count = 0;
    int *rowptr_array = create_rowptr_array(vertices, graph->numVertices, max_row, &rowptr_count);
    if (!rowptr_array) {
        free(vertices);
        free(indices_array);
        fclose(file);
        return -1;
    }
    
    int *vertex_mapping = create_vertex_mapping(vertices, graph->numVertices);
    if (!vertex_mapping) {
        free(vertices);
        free(indices_array);
        free(rowptr_array);
        fclose(file);
        return -1;
    }
    
    int *groups_array = NULL;
    int groups_count = 0;
    int *groupptr_array = NULL;
    int groupptr_count = 0;
    
    create_connection_groups(graph, vertices, vertex_mapping, 
                            &groups_array, &groups_count,
                            &groupptr_array, &groupptr_count);
    
    if (!groups_array || !groupptr_array) {
        free(vertices);
        free(indices_array);
        free(rowptr_array);
        free(vertex_mapping);
        if (groups_array) free(groups_array);
        if (groupptr_array) free(groupptr_array);
        fclose(file);
        return -1;
    }
    
    // Inicjalizacja struktury nagłówka
    BinaryFileHeader header;
    header.endianness_flag[0] = 'A';
    header.endianness_flag[1] = 'B';
    header.matrix_size = (uint16_t)graph->numCols;
    
    // Tymczasowo ustawiamy offsety na 0, zaktualizujemy je później
    header.groupptr_offset = 0;
    header.indices_offset = 0;
    header.groups_offset = 0;
    
    // Zapisz nagłówek (pozycja będzie na końcu nagłówka)
    write_binary_header(file, &header);
    
    // --- Zapisz dane ---
    
    // 1. Wskaźniki wierszy (rowptr_array) - varint + delta
    long rowptr_start = ftell(file);
    write_varint_delta(file, rowptr_array, rowptr_count);
    
    // 2. Wskaźniki grup (groupptr_array) - varint + delta
    long groupptr_start = ftell(file);
    write_varint_delta(file, groupptr_array, groupptr_count);
    
    // 3. Indeksy węzłów (indices_array) - varint
    long indices_start = ftell(file);
    write_varint_array(file, indices_array, indices_count);
    
    // 4. Grupy krawędzi (groups_array) - varint
    long groups_start = ftell(file);
    write_varint_array(file, groups_array, groups_count);
    
    // --- Zaktualizuj offsety w nagłówku i zapisz go ponownie ---
    header.groupptr_offset = (uint16_t)groupptr_start;
    header.indices_offset = (uint32_t)indices_start;
    header.groups_offset = (uint32_t)groups_start;
    
    // Wróć na początek pliku i zaktualizuj nagłówek
    fseek(file, 0, SEEK_SET);
    write_binary_header(file, &header);
    
    // Zwolnij zaalokowaną pamięć
    free(vertices);
    free(indices_array);
    free(rowptr_array);
    free(vertex_mapping);
    free(groups_array);
    free(groupptr_array);
    
    fclose(file);
    return 0;
}