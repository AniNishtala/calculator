# For WSL2/Linux. Put this file beside main.cpp and the calculator/CUDA files.
# Change CUDA_HOME if the CUDA Toolkit is installed somewhere else.
CUDA_HOME ?= /usr/local/cuda
CXX = g++
NVCC ?= nvcc

CPPFLAGS += -DCALCULATOR_USE_CUDA
CXXFLAGS ?= -std=c++11 -O2 -fopenmp -pthread
NVCCFLAGS ?= -std=c++11 -O2
LDFLAGS += -L$(CUDA_HOME)/lib64
LDLIBS += -lcudart

.PHONY: all run clean

all: calculator

calculator: main.o cuda_matrix.o
	$(CXX) $(CXXFLAGS) main.o cuda_matrix.o $(LDFLAGS) $(LDLIBS) -o $@

main.o: main.cpp calculator.h cuda_matrix.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp -o $@

cuda_matrix.o: cuda_matrix.cu cuda_matrix.h
	$(NVCC) $(NVCCFLAGS) -ccbin $(CXX) -c cuda_matrix.cu -o $@

run: calculator
	./calculator

clean:
	rm -f calculator main.o cuda_matrix.o

