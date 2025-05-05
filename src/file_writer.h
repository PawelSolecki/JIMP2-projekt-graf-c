#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include "graph.h"

// Funkcja do zapisania grafu w formacie CSRRG
int save_graph_csrrg(const Graph *graph, const char *filename);

int save_graph_bin(const Graph *graph, const char *filename);

#endif // FILE_WRITER_H