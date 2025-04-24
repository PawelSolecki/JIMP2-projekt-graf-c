#include "file_reader.h"
#include "graph.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h> //  INT_MAX
#include <ctype.h>  //  isspace

#define MAX_LINE_LENGTH 8192 

// --- Funkcja pomocnicza do parsowania linii z liczbami oddzielonymi średnikami ---
// Zwraca dynamicznie alokowaną tablicę intów i ustawia 'count'.
// Zwraca NULL w przypadku błędu.
static int* parse_semicolon_ints(const char* line, int* count) {
    *count = 0;
    if (!line) return NULL;

    const char* tmp = line;
    while (isspace((unsigned char)*tmp)) tmp++; // Pomiń początkowe białe znaki
    if (*tmp == '\0') { // Linia jest pusta lub zawiera tylko białe znaki
        *count = 0;
        int* arr = malloc(0); // Zwróć pustą, ale ważną tablicę
        return arr;
    }

    // Pierwszy przebieg: zlicz liczby
    int num_count = 0;
    const char* p = line;
    int in_number_segment = 0; // Czy jesteśmy w segmencie, który może zawierać liczbę

    while (*p) {
        if (isdigit((unsigned char)*p)) {
            if (!in_number_segment) {
                num_count++;
                in_number_segment = 1; // Zaczęliśmy potencjalną liczbę
            }
        } else if (*p == ';') {
            in_number_segment = 0; // Koniec segmentu
        } else if (isspace((unsigned char)*p)) {
            // Ignoruj białe znaki wewnątrz segmentu, ale resetuj flagę, jeśli są przed liczbą
            if (!in_number_segment && num_count > 0) {
                 // Pozwala na spacje po średniku, np. "1 ; 2"
            } else if (!isdigit((unsigned char)*(p+1)) && *(p+1) != ';' && *(p+1) != '\0' && !isspace((unsigned char)*(p+1))) {
                 // Jeśli po spacji nie ma cyfry, średnika ani końca linii, to błąd
            }
        } else if (*p == '-' || *p == '+') { // Obsługa znaku na początku liczby
             if (!in_number_segment) {
                 // Sprawdź, czy następny znak to cyfra
                 if (isdigit((unsigned char)*(p+1))) {
                     num_count++;
                     in_number_segment = 1;
                 } else {
                     fprintf(stderr, "Błąd parsowania: Znak '%c' bez następującej cyfry.\n", *p);
                     return NULL;
                 }
             } else {
                 fprintf(stderr, "Błąd parsowania: Nieoczekiwany znak '%c' wewnątrz liczby.\n", *p);
                 return NULL;
             }
        }
         else {
            // Nieprawidłowy znak
            fprintf(stderr, "Błąd parsowania: Nieoczekiwany znak '%c' w linii.\n", *p);
            return NULL;
        }
        p++;
    }

     if (num_count == 0) { // Jeśli przeszliśmy całą linię i nie znaleźliśmy liczb
         // Sprawdź, czy linia nie była po prostu pusta/białymi znakami (obsłużone na początku)
         // Jeśli zawierała coś innego niż cyfry, średniki, spacje, błąd zostałby zgłoszony wcześniej
         *count = 0;
         int* arr = malloc(0);
         return arr;
     }


    // Drugi przebieg: alokacja i parsowanie
    int* arr = malloc(num_count * sizeof(int));
    if (!arr) {
        perror("Błąd alokacji pamięci dla tablicy liczb");
        return NULL;
    }

    char* line_copy = strdup(line); 
    if (!line_copy) {
        perror("Błąd alokacji pamięci dla kopii linii");
        free(arr);
        return NULL;
    }

    char *token;
    char *rest = line_copy;
    int current_count = 0;
    char *saveptr; // Dla strtok_r

    while ((token = strtok_r(rest, ";", &saveptr)) != NULL && current_count < num_count) {
        rest = NULL; // Dla kolejnych wywołań strtok_r

        // Przytnij białe znaki z początku i końca tokenu
        while (isspace((unsigned char)*token)) token++;
        char *end = token + strlen(token) - 1;
        while (end >= token && isspace((unsigned char)*end)) *end-- = '\0';

        if (*token == '\0') continue; // Pomiń puste tokeny (np. z "1;;2")

        errno = 0;
        char *endptr;
        long val = strtol(token, &endptr, 10);

        // Sprawdź błędy konwersji strtol
        // *endptr != '\0' oznacza, że konwersja zatrzymała się przed końcem tokenu (np. "12a")
        if (errno != 0 || *endptr != '\0' || val < INT_MIN || val > INT_MAX) {
             fprintf(stderr, "Błąd parsowania: Niepoprawna wartość liczbowa '%s'.\n", token);
             free(arr);
             free(line_copy);
             return NULL;
        }
        arr[current_count++] = (int)val;
    }

    free(line_copy);

    // Sprawdzenie, czy liczba sparsowanych elementów zgadza się z oczekiwaną
    if (current_count != num_count) {
        fprintf(stderr, "Błąd wewnętrzny: Niezgodność liczby sparsowanych elementów (%d) z oczekiwaną (%d).\n", current_count, num_count);
        free(arr);
        return NULL;
    }

    *count = num_count;
    return arr;
}


// --- Funkcja pomocnicza do zwalniania zasobów w przypadku błędu ---
static void cleanup_csrrg_on_error(FILE *file, Graph *graph, int *indices, int *rowptr, int *groups, int *groupptr) {
    if (file) fclose(file);
    if (graph) freeGraph(graph); // Używa istniejącej funkcji freeGraph
    if (indices) free(indices);
    if (rowptr) free(rowptr);
    if (groups) free(groups);
    if (groupptr) free(groupptr);
}

// --- Główna funkcja do wczytania grafu z pliku w specyficznym formacie CSRRG ---
Graph* load_graph_from_file(const char* filename) {
    FILE *file = fopen(filename, "r");
    Graph *graph = NULL;
    int *indices_array = NULL, *rowptr_array = NULL, *groups_array = NULL, *groupptr_array = NULL;
    int num_indices = 0, num_rowptrs = 0, num_groups_elements = 0, num_groupptrs = 0;
    int cols = 0, rows = 0;
    char line[MAX_LINE_LENGTH];
    int line_num = 0;

    if (!file) {
        perror("Błąd otwarcia pliku");
        return NULL;
    }

    // --- Linia 1: Rozmiar macierzy (Liczba kolumn) ---
    line_num++;
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Błąd: Nie można odczytać linii %d (Rozmiar macierzy/Kolumny) z pliku %s.\n", line_num, filename);
        cleanup_csrrg_on_error(file, NULL, NULL, NULL, NULL, NULL);
        return NULL;
    }
    errno = 0;
    char *endptr;
    long val = strtol(line, &endptr, 10);
    while (isspace((unsigned char)*endptr)) endptr++;
    if (errno != 0 || *endptr != '\0' || val <= 0 || val > INT_MAX) {
         fprintf(stderr, "Błąd: Niepoprawny format lub wartość w linii %d (Rozmiar macierzy/Kolumny > 0) pliku %s.\n", line_num, filename);
         cleanup_csrrg_on_error(file, NULL, NULL, NULL, NULL, NULL);
         return NULL;
    }
    cols = (int)val;
    rows = cols; // Zakładamy macierz kwadratową

    // --- Linia 2: Indeksy węzłów w poszczególnych wierszach ---
    line_num++;
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Błąd: Nie można odczytać linii %d (Indeksy węzłów) z pliku %s.\n", line_num, filename);
        cleanup_csrrg_on_error(file, NULL, NULL, NULL, NULL, NULL);
        return NULL;
    }
    indices_array = parse_semicolon_ints(line, &num_indices);
    if (!indices_array && num_indices == 0) {
        const char* tmp = line;
        while(isspace((unsigned char)*tmp)) tmp++;
        if (*tmp != '\0') {
             fprintf(stderr, "Błąd: Nie udało się sparsować linii %d (Indeksy węzłów) w pliku %s.\n", line_num, filename);
             cleanup_csrrg_on_error(file, NULL, NULL, NULL, NULL, NULL);
             return NULL;
        }
    }
    for (int i = 0; i < num_indices; ++i) {
        if (indices_array[i] < 0 || indices_array[i] >= cols) {
             fprintf(stderr, "Błąd: Nieprawidłowy indeks kolumny %d (poza zakresem [0, %d)) w linii %d pliku %s.\n", indices_array[i], cols, line_num, filename);
             cleanup_csrrg_on_error(file, NULL, indices_array, NULL, NULL, NULL);
             return NULL;
        }
    }

    // --- Linia 3: Wskaźniki na pierwszy węzeł w wierszu ---
    line_num++;
     if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Błąd: Nie można odczytać linii %d (Wskaźniki wierszy) z pliku %s.\n", line_num, filename);
        cleanup_csrrg_on_error(file, NULL, indices_array, NULL, NULL, NULL);
        return NULL;
    }
    rowptr_array = parse_semicolon_ints(line, &num_rowptrs);
     if (!rowptr_array && num_rowptrs == 0) {
        const char* tmp = line;
        while(isspace((unsigned char)*tmp)) tmp++;
         if (*tmp != '\0') {
             fprintf(stderr, "Błąd: Nie udało się sparsować linii %d (Wskaźniki wierszy) w pliku %s.\n", line_num, filename);
             cleanup_csrrg_on_error(file, NULL, indices_array, NULL, NULL, NULL);
             return NULL;
         }
    }
    if (num_rowptrs != rows + 1) {
        fprintf(stderr, "Błąd: Linia %d (Wskaźniki wierszy) powinna zawierać %d elementów (rows+1), a zawiera %d w pliku %s.\n", line_num, rows + 1, num_rowptrs, filename);
        cleanup_csrrg_on_error(file, NULL, indices_array, rowptr_array, NULL, NULL);
        return NULL;
    }

    // --- Zlicz faktyczną liczbę wierzchołków i utwórz mapowania ---
    int actual_vertex_count = 0;
    for (int row = 0; row < rows; row++) {
        int start_idx = rowptr_array[row];
        int end_idx = rowptr_array[row+1];
        actual_vertex_count += (end_idx - start_idx); // Dodaj liczbę wierzchołków w tym wierszu
    }
    
    // Utwórz mapowania pozycji wierzchołków
    int* positions = malloc(num_indices * 2 * sizeof(int)); // [row1, col1, row2, col2, ...]
    int vertex_index = 0;
    
    for (int row = 0; row < rows; row++) {
        int start_idx = rowptr_array[row];
        int end_idx = rowptr_array[row+1];
        
        for (int j = start_idx; j < end_idx; j++) {
            int col = indices_array[j];
            // Zapisz pozycję dla każdego wierzchołka
            positions[vertex_index*2] = row;
            positions[vertex_index*2+1] = col;
            vertex_index++;
        }
    }
    
    // Utwórz graf o faktycznej liczbie wierzchołków
    graph = createGraph(actual_vertex_count);
    graph->numCols = cols; // Ustaw liczbę kolumn

    // --- Wczytaj linie 4 i 5 i przetwórz grupy połączeń ---
    line_num++;
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Błąd: Nie można odczytać linii %d (Grupy połączeń) z pliku %s.\n", line_num, filename);
        cleanup_csrrg_on_error(file, graph, indices_array, rowptr_array, NULL, NULL);
        free(positions);
        return NULL;
    }
    groups_array = parse_semicolon_ints(line, &num_groups_elements);
    
    line_num++;
    if (fgets(line, sizeof(line), file) == NULL) {
        fprintf(stderr, "Błąd: Nie można odczytać linii %d (Wskaźniki grup) z pliku %s.\n", line_num, filename);
        cleanup_csrrg_on_error(file, graph, indices_array, rowptr_array, groups_array, NULL);
        free(positions);
        return NULL;
    }
    groupptr_array = parse_semicolon_ints(line, &num_groupptrs);

    // --- Budowanie krawędzi grafu z uwzględnieniem pozycji ---
    if (num_groupptrs > 0) {
        int num_groups = num_groupptrs - 1;
        for (int i = 0; i < num_groups; ++i) {
            int start_idx = groupptr_array[i];
            int end_idx = groupptr_array[i+1];

            if (start_idx >= end_idx) continue; // Pomiń pustą grupę

            int src_vertex_idx_in_array = groups_array[start_idx]; // Indeks w oryginalnej numeracji
            int src_vertex = -1;
            int src_row = -1, src_col = -1;
            
            // Znajdź mapowanie z oryginalnego indeksu na nowy sekwencyjny indeks
            for (int r = 0; r < rows && src_vertex == -1; r++) {
                int start = rowptr_array[r];
                int end = rowptr_array[r+1];
                for (int j = start; j < end; j++) {
                    if (j == src_vertex_idx_in_array) {
                        // To jest wierzchołek, którego szukamy
                        src_vertex = j - start; // Nowy sekwencyjny indeks
                        for (int k = 0; k < actual_vertex_count; k++) {
                            if (positions[k*2] == r && positions[k*2+1] == indices_array[j]) {
                                src_vertex = k;
                                src_row = r;
                                src_col = indices_array[j];
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            
            if (src_vertex == -1) {
                fprintf(stderr, "Błąd: Nie można znaleźć pozycji dla wierzchołka %d w pliku %s.\n", src_vertex_idx_in_array, filename);
                continue;
            }
            
            // Dodaj krawędzie od src_vertex do wszystkich pozostałych wierzchołków w grupie
            for (int j = start_idx + 1; j < end_idx; ++j) {
                int dest_vertex_idx_in_array = groups_array[j];
                int dest_vertex = -1;
                int dest_row = -1, dest_col = -1;
                
                // Znajdź mapowanie z oryginalnego indeksu na nowy sekwencyjny indeks
                for (int r = 0; r < rows && dest_vertex == -1; r++) {
                    int start = rowptr_array[r];
                    int end = rowptr_array[r+1];
                    for (int k = start; k < end; k++) {
                        if (k == dest_vertex_idx_in_array) {
                            // To jest wierzchołek, którego szukamy
                            for (int m = 0; m < actual_vertex_count; m++) {
                                if (positions[m*2] == r && positions[m*2+1] == indices_array[k]) {
                                    dest_vertex = m;
                                    dest_row = r;
                                    dest_col = indices_array[k];
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                
                if (dest_vertex == -1) {
                    fprintf(stderr, "Błąd: Nie można znaleźć pozycji dla wierzchołka %d w pliku %s.\n", dest_vertex_idx_in_array, filename);
                    continue;
                }
                
                // Dodaj krawędź z informacją o pozycjach
                addEdge(graph, src_vertex, src_row, src_col, dest_vertex, dest_row, dest_col);
            }
        }
    }

    // --- Sprzątanie i zwrot ---
    fclose(file);
    free(indices_array);
    free(rowptr_array);
    free(groups_array);
    free(groupptr_array);
    free(positions);

    return graph;
}