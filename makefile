# Linux / WSL2
CUDA_HOME ?= /usr/local/cuda
CXX = g++
NVCC ?= $(if $(wildcard $(CUDA_HOME)/bin/nvcc),$(CUDA_HOME)/bin/nvcc,nvcc)

CXXFLAGS ?= -std=c++11 -O2 -fopenmp -pthread
NVCCFLAGS ?= -std=c++11 -O2
LDLIBS += -lX11

OBJECTS = main.o gui.o plot_test.o

# Enable CUDA only when nvcc is available.
ifneq ($(shell command -v $(NVCC) 2>/dev/null),)
CPPFLAGS += -DCALCULATOR_USE_CUDA -I$(CUDA_HOME)/include
LDFLAGS += -L$(CUDA_HOME)/lib64
LDLIBS += -lcudart
OBJECTS += cuda_matrix.o
endif

.PHONY: all build run plot clean

all: calculator

build: calculator

calculator: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

main.o: main.cpp calculator.h cuda_matrix.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp -o $@

gui.o: gui.cpp gui.h calculator.h cuda_matrix.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c gui.cpp -o $@

plot_test.o: plot_test.cpp calculator.h cuda_matrix.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c plot_test.cpp -o $@

cuda_matrix.o: cuda_matrix.cu cuda_matrix.h
	$(NVCC) $(NVCCFLAGS) -ccbin $(CXX) -c cuda_matrix.cu -o $@

plot_test: plot_test_standalone.o
	$(CXX) $(CXXFLAGS) plot_test_standalone.o -lX11 -o $@

# Change this macro if your source uses a different standalone guard.
plot_test_standalone.o: plot_test.cpp calculator.h cuda_matrix.h
	$(CXX) $(CXXFLAGS) -DPLOT_TEST_STANDALONE -c plot_test.cpp -o $@

run: calculator
	./calculator

plot: plot_test
	./plot_test

clean:
	rm -f calculator plot_test *.o
