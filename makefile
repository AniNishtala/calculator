# WSL2/Linux. Change CUDA_HOME if your CUDA Toolkit is installed elsewhere.
CUDA_HOME ?= /usr/local/cuda
CXX = g++
NVCC ?= nvcc

CPPFLAGS += -DCALCULATOR_USE_CUDA
CXXFLAGS ?= -std=c++11 -O2 -fopenmp -pthread
NVCCFLAGS ?= -std=c++11 -O2
LDFLAGS += -L$(CUDA_HOME)/lib64
LDLIBS += -lcudart -lX11

.PHONY: all run plot clean

all: calculator

calculator: main.o gui.o plot_test.o cuda_matrix.o
	$(CXX) $(CXXFLAGS) main.o gui.o plot_test.o cuda_matrix.o $(LDFLAGS) $(LDLIBS) -o $@

main.o: main.cpp calculator.h cuda_matrix.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp -o $@

gui.o: gui.cpp gui.h calculator.h cuda_matrix.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c gui.cpp -o $@

cuda_matrix.o: cuda_matrix.cu cuda_matrix.h
	$(NVCC) $(NVCCFLAGS) -ccbin $(CXX) -c cuda_matrix.cu -o $@

# Standalone 16-curve plotter: CPU, pthreads, OpenMP, and X11 only.
plot_test: plot_test_standalone.o
	$(CXX) $(CXXFLAGS) plot_test_standalone.o $(LDFLAGS) -lX11 -o $@

plot_test.o: plot_test.cpp calculator.h cuda_matrix.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c plot_test.cpp -o $@

plot_test_standalone.o: plot_test.cpp calculator.h
	$(CXX) $(filter-out -DCALCULATOR_USE_CUDA,$(CPPFLAGS)) $(CXXFLAGS) -DPLOT_TEST_STANDALONE -c plot_test.cpp -o $@

plot: plot_test
	./plot_test

run: calculator
	./calculator

clean:
	rm -f calculator plot_test main.o gui.o cuda_matrix.o plot_test.o plot_test_standalone.o

