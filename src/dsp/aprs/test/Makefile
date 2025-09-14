# Minimal Makefile for APRS Benchmark
SRC_DIR = ../../..
CXX = g++
CXXFLAGS = -std=c++17 -O3 -I$(SRC_DIR) -I. -DLOG=printf -DBENCHMARK_BUILD
TARGET = benchmark

# Your actual source files (add/remove as needed)
SOURCES = benchmark.cpp 


$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) -lm

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Usage: ./$(TARGET) your_file.wav"

.PHONY: clean test
