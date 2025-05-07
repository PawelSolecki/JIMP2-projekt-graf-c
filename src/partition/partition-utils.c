#include "partition-utils.h"

#include <stdlib.h>
#include <stdio.h>

DynamicIntList* createDynamicList() {
    DynamicIntList* list = malloc(sizeof(DynamicIntList));
    list->capacity = 4;
    list->size = 0;
    list->data = malloc(list->capacity * sizeof(int));
    return list;
}

void addToDynamicList(DynamicIntList* list, int value) {
    if (list->size >= list->capacity) {
        list->capacity *= 2;
        list->data = realloc(list->data, list->capacity * sizeof(int));
    }
    list->data[list->size++] = value;
}

void freeDynamicList(DynamicIntList* list) {
    if (list) {
        free(list->data);
        free(list);
    }
}

int** convertTo2DArray(DynamicIntList** map, int size, int** sizesOut) {
    int** out = malloc(size * sizeof(int*));
    int* sizes = malloc(size * sizeof(int));

    for (int i = 0; i < size; i++) {
        sizes[i] = map[i]->size;
        out[i] = malloc(sizes[i] * sizeof(int));
        for (int j = 0; j < sizes[i]; j++) {
            out[i][j] = map[i]->data[j];
        }
    }

    *sizesOut = sizes;
    return out;
}
