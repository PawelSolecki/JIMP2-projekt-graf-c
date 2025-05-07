#include "file_writer.h"
#include "file_reader.h"
#include "utils.h"
#include "partition/partitioning.h"
#include "partition/coarsening.h"
#include "partition/refinement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SCALE_FACTOR 10
#define DEFAULT_PARTS 2
#define DEFAULT_MARGIN 10
#define DEFAULT_OUTPUT_FORMAT "csrrg"
#define DEFAULT_FILENAME "data/output.csrrg"

int main(int argc, char *argv[]) {
    // Wczytaj graf z pliku wejściowego
    Graph *graph = load_graph_from_file(argv[1]);
    if (!graph) {
        fprintf(stderr, "Nie udało się wczytać grafu z pliku %s.\n", argv[1]);
        return EXIT_FAILURE;
    }
    printGraph(graph);

    int parts = -1; int margin = -1;
    char *output_format = NULL; char *filename = NULL;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--", 2) == 0) {
            char *flag = argv[i];
            if (i + 1 < argc && strncmp(argv[i + 1], "--", 2) != 0) {
                char *value = argv[i + 1];
                printf("Flag: %s, Value: %s\n", flag, value);
                i++;
                switch (flag[2]) {
                    case 'p':
                        parts = parts_flag(value, graph->numVertices);
                        if (parts == -1) {
                            fprintf(stderr, "Liczba podziałów musi być większa od 0 i mniejsza od liczby wierzchołkow. Wczytano: \"%s\".\n", value);
                            freeGraph(graph);
                            return EXIT_FAILURE;
                        }
                        break;
                    case 'm':
                        margin = margin_flag(value);
                        if (margin == -1) {
                            fprintf(stderr, "Margines procentowy musi być w zakresie 0-100. Wczytano: \"%s\".\n", value);
                            freeGraph(graph);
                            return EXIT_FAILURE;
                        }
                        break;
                    case 'o':
                        output_format = output_flag(value);
                        if (output_format == NULL) {
                            fprintf(stderr, "Program obsługuje formaty csrrg i bin. Wczytano: \"%s\".\n", value);
                            freeGraph(graph);
                            return EXIT_FAILURE;
                        }
                        break;
                    case 'f':
                        filename = filename_flag(value);
                        if (filename == NULL) {
                            fprintf(stderr, "Nie można przydzielić pamięci dla nazwy pliku.\n");
                            freeGraph(graph);
                            return EXIT_FAILURE;
                        }
                        break;
                    default:
                        printf("Nieznana flaga: %s\n", flag);
                }
            } else {
                fprintf(stderr, "Nie podano wartości dla flagi: %s\n", flag);
                freeGraph(graph);
                return EXIT_FAILURE;
            }
        }
    }

    if (parts == -1) parts = DEFAULT_PARTS;
    if (margin == -1) margin = DEFAULT_MARGIN;
    if (output_format == NULL) output_format = DEFAULT_OUTPUT_FORMAT;
    if (filename == NULL) filename = DEFAULT_FILENAME;

    // Podziel graf na części
    int **coarseToFineMap = NULL;

    Graph *coarse = coarsenGraph(graph, (int) parts * SCALE_FACTOR, &coarseToFineMap);
    int *partition = initialPartition(coarse, parts);
    refinePartition(graph, partition, parts, margin, coarseToFineMap);

    graph->partitions = partition;
    graph = refreshGraphWithPartitions(graph);

    printGraph(graph);

    // Zapisz graf do pliku wyjściowego
    if ((strcmp(output_format, "csrrg") == 0 && save_graph_csrrg(graph, filename) != 0) ||
        (strcmp(output_format, "bin") == 0 && save_graph_bin(graph, filename) != 0)) {
        fprintf(stderr, "Nie udało się zapisać grafu do pliku %s.\n", filename);
        freeGraph(graph);
        return EXIT_FAILURE;
    }  

    printf("Graf został pomyślnie zapisany do pliku %s.\n", filename);
    freeGraph(graph);
    return EXIT_SUCCESS;
}