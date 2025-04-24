#include <stdio.h>
#include <stdlib.h>
#include "graph.h"       // Definicje struktur Graph, Node i funkcji createGraph, addEdge, printGraph, freeGraph, getVertexPosition, getVertexIndex
#include "file_reader.h" // Deklaracja funkcji load_graph_from_file

int main(int argc, char *argv[]) {
    // Sprawdzenie, czy podano dokładnie jeden argument (nazwę pliku)
    if (argc != 2) {
        fprintf(stderr, "Użycie: %s <ścieżka_do_pliku.csrrg>\n", argv[0]);
        return EXIT_FAILURE; // Zwróć kod błędu
    }

    const char* filename = argv[1]; // Pobierz nazwę pliku z argumentów
    Graph *graph = NULL;

    printf("Wczytywanie grafu z pliku: %s\n", filename);

    // Wczytaj graf z pliku za pomocą funkcji z csrrg_reader.c
    graph = load_graph_from_file(filename);

    // Sprawdź, czy wczytywanie się powiodło
    if (graph == NULL) {
        fprintf(stderr, "Nie udało się wczytać grafu z pliku %s.\n", filename);
        // Funkcja load_graph_from_file powinna już wypisać szczegółowy błąd
        return EXIT_FAILURE; // Zwróć kod błędu
    }

    printf("\nGraf wczytany pomyślnie.\n");

    // Wypisz wczytany graf na konsolę
    printGraph(graph);


    // Zwolnij pamięć zajmowaną przez graf
    printf("\nZwalnianie pamięci grafu...\n");
    freeGraph(graph);
    printf("Pamięć zwolniona.\n");

    return EXIT_SUCCESS; // Zwróć kod sukcesu
}