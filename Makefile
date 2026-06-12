CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -pedantic -fopenmp

TARGET := bin/parallel_apsp
SOURCES := src/parallel_apsp.cpp

.PHONY: all clean test benchmark

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $< -o $@

test: $(TARGET)
	bash scripts/run_tests.sh

benchmark: $(TARGET)
	bash scripts/run_benchmark.sh

clean:
	rm -rf bin build results
