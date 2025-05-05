#include "file_writer.h"
#include "file_reader.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Użycie: %s <plik_wejściowy.csrrg> <plik_wyjściowy.csrrg>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Wczytaj graf z pliku wejściowego
    Graph *graph = load_graph_from_file(argv[1]);
    if (!graph) {
        fprintf(stderr, "Nie udało się wczytać grafu z pliku %s.\n", argv[1]);
        return EXIT_FAILURE;
    }
    printGraph(graph);

    // Zapisz graf do pliku wyjściowego
    if (save_graph_bin(graph, argv[2]) != 0) {
        fprintf(stderr, "Nie udało się zapisać grafu do pliku %s.\n", argv[2]);
        freeGraph(graph);
        return EXIT_FAILURE;
    }

    printf("Graf został pomyślnie zapisany do pliku %s.\n", argv[2]);
    freeGraph(graph);
    return EXIT_SUCCESS;
}