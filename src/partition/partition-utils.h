#ifndef PARTITION_UTILS_H
#define PARTITION_UTILS_H

typedef struct {
    int* data;
    int size;
    int capacity;
} DynamicIntList;

DynamicIntList* createDynamicList();

void addToDynamicList(DynamicIntList* list, int value);

void freeDynamicList(DynamicIntList* list);

int** convertTo2DArray(DynamicIntList** map, int size, int** sizesOut);

#endif // PARTITION_UTILS_H