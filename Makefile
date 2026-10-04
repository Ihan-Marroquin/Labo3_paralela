MPICC ?= mpicc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c11
BUILD_DIR := build
PROGRAMS := ping_pong token_ring recepcion_anticipada pipeline_chunks
TARGETS := $(PROGRAMS:%=$(BUILD_DIR)/%)

.PHONY: all clean resultados

all: $(TARGETS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%: src/%.c | $(BUILD_DIR)
	$(MPICC) $(CFLAGS) $< -o $@

resultados: all
	bash scripts/ejecutar_experimentos.sh
	python3 scripts/graficar_resultados.py

clean:
	rm -rf $(BUILD_DIR)
