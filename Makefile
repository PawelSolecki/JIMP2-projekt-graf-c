# Uproszczony Makefile dla projektu grafu

# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -lm

# Ścieżki plików
SRC_DIR = src
BUILD_DIR = build
EXECUTABLE = graph_reader

# Pliki źródłowe
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/graph.c $(SRC_DIR)/csrrg_reader.c $(SRC_DIR)/csrrg_writer.c

# Pliki obiektowe
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/graph.o $(BUILD_DIR)/csrrg_reader.o $(BUILD_DIR)/csrrg_writer.o

# Główny cel - kompilacja programu
all: $(BUILD_DIR) $(EXECUTABLE)

# Tworzenie katalogu build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Kompilacja pliku wykonywalnego
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE) -lm	

# Reguły kompilacji dla poszczególnych plików
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $(BUILD_DIR)/main.o

$(BUILD_DIR)/graph.o: $(SRC_DIR)/graph.c $(SRC_DIR)/graph.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/graph.c -o $(BUILD_DIR)/graph.o

$(BUILD_DIR)/csrrg_reader.o: $(SRC_DIR)/csrrg_reader.c $(SRC_DIR)/file_reader.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/csrrg_reader.c -o $(BUILD_DIR)/csrrg_reader.o

$(BUILD_DIR)/csrrg_writer.o: $(SRC_DIR)/csrrg_writer.c $(SRC_DIR)/file_writer.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/csrrg_writer.c -o $(BUILD_DIR)/csrrg_writer.o

# Uruchamianie programu (sztywne ścieżki)
run: all
	./$(EXECUTABLE) data/graf1.csrrg data/output.csrrg

# Czyszczenie projektu
clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)

.PHONY: all clean run