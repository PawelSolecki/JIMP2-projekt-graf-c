#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *ALLOWED_FILETYPES[] = {"bin", "csrrg"};
const int ALLOWED_FILETYPES_COUNT = 2;

int parts_flag(char *value, int num_vertices) {
    if (value == NULL) return -1;
    int parts = atoi(value);
    return parts <= 0 || parts >= num_vertices ? -1 : parts;
}

int margin_flag(char *value) {
    if (value == NULL) return -1;
    int margin = atoi(value);
    return margin < 0 || margin > 100 ? -1 : margin;
}

char * output_flag(char *value) {
    if (value == NULL) return NULL;
    
    for (int i = 0; i < ALLOWED_FILETYPES_COUNT; i++) {
        if (strcmp(value, ALLOWED_FILETYPES[i]) == 0) {
            return value;
        }
    }
    return NULL;
}

char * filename_flag(char *value) {
    if (value == NULL) return NULL;
    char *filename = malloc(strlen(value) + 1);
    if (filename == NULL) {
        fprintf(stderr, "Nie można przydzielić pamięci dla nazwy pliku.\n");
        return NULL;
    }
    strcpy(filename, value);
    return filename;
}