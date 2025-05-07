# Uproszczony Makefile dla projektu grafu

# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -lm

# Ścieżki plików
SRC_DIR = src
BUILD_DIR = build
EXECUTABLE = graph_reader

# Pliki źródłowe
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/graph.c $(SRC_DIR)/csrrg_reader.c $(SRC_DIR)/csrrg_writer.c $(SRC_DIR)/bin_writer.c $(SRC_DIR)/utils.c $(SRC_DIR)/partition/partitioning.c

# Pliki obiektowe
OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/graph.o $(BUILD_DIR)/csrrg_reader.o $(BUILD_DIR)/csrrg_writer.o $(BUILD_DIR)/bin_writer.o $(BUILD_DIR)/utils.o $(BUILD_DIR)/partitioning.o

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

$(BUILD_DIR)/bin_writer.o: $(SRC_DIR)/bin_writer.c $(SRC_DIR)/file_writer.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/bin_writer.c -o $(BUILD_DIR)/bin_writer.o

$(BUILD_DIR)/utils.o: $(SRC_DIR)/utils.c $(SRC_DIR)/utils.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/utils.c -o $(BUILD_DIR)/utils.o

$(BUILD_DIR)/partitioning.o: $(SRC_DIR)/partition/partitioning.c $(SRC_DIR)/partition/partitioning.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/partition/partitioning.c -o $(BUILD_DIR)/partitioning.o

# Uruchamianie programu (sztywne ścieżki)
run: all
	@if [ -z "$(FILENAME)" ]; then \
		echo "Error: FILENAME is not set. Please provide FILENAME=<path>"; \
		exit 1; \
	fi; \
	if [ -z "$(OUTPUT)" ]; then \
		echo "Warning: OUTPUT is not set. Defaulting to csrrg."; \
		export OUTPUT=csrrg; \
	fi; \
	if [ "$(OUTPUT)" = "csrrg" ]; then \
		./$(EXECUTABLE) data/graf1.csrrg --output csrrg --filename $(FILENAME).csrrg; \
	elif [ "$(OUTPUT)" = "bin" ]; then \
		./$(EXECUTABLE) data/graf1.csrrg --output bin --filename $(FILENAME).bin; \
	else \
		echo "Error: Wskaż parametry OUTPUT=bin or OUTPUT=csrrg oraz FILENAME=<ścieżka>"; \
	fi


# Czyszczenie projektu
clean:
	rm -rf $(BUILD_DIR) $(EXECUTABLE)

.PHONY: all clean run